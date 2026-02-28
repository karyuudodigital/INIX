/*
    File: app/MainWindow.cpp
    Purpose:
      - Implements the top-level UI composition and interaction logic for INIX.

    How it fits in the codebase:
      - Owns concrete widgets, models, and service instances.
      - Connects menu/button/search events to domain/service operations.
      - Launches parse/diff work on background threads via QtConcurrent and updates UI on completion.

    Design notes:
      - Data logic remains in domain/services. This file focuses on orchestration.
      - Qt object ownership uses parent-child relationships to avoid manual delete calls.
*/

#include "app/MainWindow.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QFileDialog>
#include <QDir>
#include <QFuture>
#include <QHeaderView>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QFileInfo>
#include <QPushButton>
#include <QRegularExpression>
#include <QStatusBar>
#include <QTableView>
#include <QTabWidget>
#include <QTextEdit>
#include <QToolBar>
#include <QUndoCommand>
#include <QVBoxLayout>
#include <QPalette>
#include <QStyleFactory>
#include <QtConcurrent>

#include <algorithm>

namespace {
// Shared visual metrics for dock titles and table headers.
constexpr int kMainHeaderHeight = 36;
constexpr int kDockTitleVerticalPadding = 10;

struct SnippetEntry {
    // Represents one semantic INI row used while transforming preset snippets.
    // This is intentionally independent from IniLine because presets are a compact
    // transport format (section + key + value) and do not carry formatting metadata.
    QString section;
    QString key;
    QString value;
};

QString entryToken(const QString& section, const QString& key) {
    // Case-insensitive identity for "same logical setting".
    // This is used for merge dedupe and updates.
    return section.toLower() + QStringLiteral("||") + key.toLower();
}

QVector<SnippetEntry> parseSnippetEntries(const QString& snippet) {
    // Parse the lightweight preset text format:
    // [Section]
    // key=value
    // key2=value2
    //
    // The parser is permissive:
    // - blank and comment lines are ignored
    // - malformed lines are skipped
    // - key/value lines only apply once a section header is seen
    QVector<SnippetEntry> entries;
    QString currentSection;

    const QStringList lines = snippet.split(QRegularExpression("\r\n|\n|\r"), Qt::KeepEmptyParts);
    for (const QString& rawLine : lines) {
        const QString trimmed = rawLine.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith(';') || trimmed.startsWith('#')) {
            continue;
        }
        if (trimmed.startsWith('[') && trimmed.endsWith(']') && trimmed.size() > 2) {
            currentSection = trimmed.mid(1, trimmed.size() - 2).trimmed();
            continue;
        }

        const int equalsPos = rawLine.indexOf('=');
        if (equalsPos <= 0 || currentSection.isEmpty()) {
            continue;
        }
        const QString key = rawLine.left(equalsPos).trimmed();
        if (key.isEmpty()) {
            continue;
        }

        entries.push_back(SnippetEntry{
            .section = currentSection,
            .key = key,
            .value = rawLine.mid(equalsPos + 1),
        });
    }
    return entries;
}

QString buildSnippetFromEntries(const QVector<SnippetEntry>& entries) {
    // Convert normalized entries back into grouped INI text that can be saved as
    // a preset value in JSON. Ordering is stable by insertion order.
    if (entries.isEmpty()) {
        return {};
    }

    struct SectionBucket {
        QString sectionName;
        QStringList lines;
    };

    QVector<SectionBucket> buckets;
    QHash<QString, int> sectionIndex;
    for (const auto& entry : entries) {
        const QString token = entry.section.toLower();
        int bucketIndex = sectionIndex.value(token, -1);
        if (bucketIndex < 0) {
            bucketIndex = buckets.size();
            sectionIndex.insert(token, bucketIndex);
            buckets.push_back(SectionBucket{
                .sectionName = entry.section,
                .lines = {},
            });
        }
        buckets[bucketIndex].lines.push_back(QStringLiteral("%1=%2").arg(entry.key, entry.value));
    }

    QStringList out;
    for (int i = 0; i < buckets.size(); ++i) {
        out.push_back(QStringLiteral("[%1]").arg(buckets[i].sectionName));
        out.append(buckets[i].lines);
        if (i + 1 < buckets.size()) {
            out.push_back(QString());
        }
    }
    return out.join(QStringLiteral("\n"));
}

// Undo command that restores full-document snapshots before/after merge apply.
// This keeps undo implementation simple and robust.
class MergeApplyCommand final : public QUndoCommand {
public:
    MergeApplyCommand(IniDocument* document, IniDocumentSnapshot before, IniDocumentSnapshot after)
        : document_(document), before_(std::move(before)), after_(std::move(after)) {
        setText("Apply merge");
    }

    void undo() override { document_->restore(before_); }
    void redo() override { document_->restore(after_); }

private:
    IniDocument* document_ = nullptr;
    IniDocumentSnapshot before_;
    IniDocumentSnapshot after_;
};
} // namespace

// ----- Construction and UI composition -----

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      settingsModel_(this),
      filterProxyModel_(this),
      diffTableModel_(this),
      mergePreviewModel_(this) {
    // Build widget tree and interaction wiring first.
    setupUi();
    setupMenus();
    setupConnections();
    applyDarkTheme();

    // Connect domain document to table models/proxies.
    settingsModel_.setDocument(&document_);
    filterProxyModel_.setSourceModel(&settingsModel_);
    settingsTable_->setModel(&filterProxyModel_);

    diffTable_->setModel(&diffTableModel_);
    mergeTable_->setModel(&mergePreviewModel_);

    updateWindowTitle();
    statusBar()->showMessage("Ready");
}

void MainWindow::setupUi() {
    // Window base size. Users can resize freely after startup.
    resize(1280, 820);

    // Search toolbar drives proxy filtering.
    auto* searchToolbar = addToolBar("Search");
    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText("Search section/key/value...");
    caseSensitiveCheck_ = new QCheckBox("Case", this);
    regexCheck_ = new QCheckBox("Regex", this);
    searchCountLabel_ = new QLabel("Matches: 0", this);

    searchToolbar->addWidget(new QLabel("Search:", this));
    searchToolbar->addWidget(searchEdit_);
    searchToolbar->addWidget(caseSensitiveCheck_);
    searchToolbar->addWidget(regexCheck_);
    searchToolbar->addWidget(searchCountLabel_);

    // Central table: target INI settings.
    settingsTable_ = new QTableView(this);
    settingsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    settingsTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    settingsTable_->setAlternatingRowColors(true);
    settingsTable_->horizontalHeader()->setStretchLastSection(true);
    settingsTable_->horizontalHeader()->setFixedHeight(kMainHeaderHeight);
    setCentralWidget(settingsTable_);

    // Left dock: opened file list/history.
    filesDock_ = new QDockWidget("Files", this);
    filesDock_->setObjectName("filesDock");
    filesDock_->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable |
                            QDockWidget::DockWidgetFloatable);
    fileList_ = new QListWidget(filesDock_);
    filesDock_->setWidget(fileList_);
    addDockWidget(Qt::LeftDockWidgetArea, filesDock_);

    // Right dock: diff and merge workflows in a tab widget.
    diffMergeDock_ = new QDockWidget("Diff / Merge", this);
    diffMergeDock_->setObjectName("diffMergeDock");
    diffMergeDock_->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable |
                                QDockWidget::DockWidgetFloatable);
    auto* tabs = new QTabWidget(diffMergeDock_);

    auto* diffTab = new QWidget(tabs);
    auto* diffLayout = new QVBoxLayout(diffTab);
    auto* loadCompareButton = new QPushButton("Load Compare INI", diffTab);
    auto* recomputeDiffButton = new QPushButton("Build Diff", diffTab);
    diffSummaryLabel_ = new QLabel("Diff: not computed", diffTab);
    diffTable_ = new QTableView(diffTab);
    diffTable_->horizontalHeader()->setStretchLastSection(true);
    diffTable_->horizontalHeader()->setFixedHeight(kMainHeaderHeight);
    diffLayout->addWidget(loadCompareButton);
    diffLayout->addWidget(recomputeDiffButton);
    diffLayout->addWidget(diffSummaryLabel_);
    diffLayout->addWidget(diffTable_);
    tabs->addTab(diffTab, "Diff");

    auto* mergeTab = new QWidget(tabs);
    auto* mergeLayout = new QVBoxLayout(mergeTab);
    auto* previewButton = new QPushButton("Prepare Merge Preview", mergeTab);
    auto* selectAllButton = new QPushButton("Select All", mergeTab);
    auto* selectChangedButton = new QPushButton("Select Changed", mergeTab);
    auto* selectAddedButton = new QPushButton("Select Added", mergeTab);
    auto* clearSelectionButton = new QPushButton("Clear Selection", mergeTab);
    conflictPolicyCombo_ = new QComboBox(mergeTab);
    conflictPolicyCombo_->addItems({"Replace target", "Keep target", "Prompt each conflict"});
    auto* applyMergeButton = new QPushButton("Apply Merge", mergeTab);
    mergeTable_ = new QTableView(mergeTab);
    mergeTable_->horizontalHeader()->setStretchLastSection(true);
    mergeTable_->horizontalHeader()->setFixedHeight(kMainHeaderHeight);

    mergeLayout->addWidget(previewButton);
    mergeLayout->addWidget(selectAllButton);
    mergeLayout->addWidget(selectChangedButton);
    mergeLayout->addWidget(selectAddedButton);
    mergeLayout->addWidget(clearSelectionButton);
    mergeLayout->addWidget(new QLabel("Conflict policy:", mergeTab));
    mergeLayout->addWidget(conflictPolicyCombo_);
    mergeLayout->addWidget(applyMergeButton);
    mergeLayout->addWidget(mergeTable_);
    tabs->addTab(mergeTab, "Merge");

    diffMergeDock_->setWidget(tabs);
    addDockWidget(Qt::RightDockWidgetArea, diffMergeDock_);

    // Bottom dock: append-only status log.
    statusDock_ = new QDockWidget("Status / Log", this);
    statusDock_->setObjectName("statusDock");
    statusDock_->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable |
                             QDockWidget::DockWidgetFloatable);
    logOutput_ = new QTextEdit(statusDock_);
    logOutput_->setReadOnly(true);
    statusDock_->setWidget(logOutput_);
    addDockWidget(Qt::BottomDockWidgetArea, statusDock_);

    // Object names are used for findChild wiring in setupConnections().
    loadCompareButton->setObjectName("loadCompareButton");
    recomputeDiffButton->setObjectName("recomputeDiffButton");
    previewButton->setObjectName("previewMergeButton");
    applyMergeButton->setObjectName("applyMergeButton");
    selectAllButton->setObjectName("selectAllMergeButton");
    selectChangedButton->setObjectName("selectChangedMergeButton");
    selectAddedButton->setObjectName("selectAddedMergeButton");
    clearSelectionButton->setObjectName("clearMergeSelectionButton");
}

void MainWindow::setupMenus() {
    // File menu contains open/save lifecycle actions.
    auto* fileMenu = menuBar()->addMenu("&File");
    auto* openAction = fileMenu->addAction("Open...");
    auto* saveAction = fileMenu->addAction("Save");
    auto* saveAsAction = fileMenu->addAction("Save As...");
    fileMenu->addSeparator();
    auto* openCompareAction = fileMenu->addAction("Open Compare INI...");
    fileMenu->addSeparator();
    auto* quitAction = fileMenu->addAction("Quit");

    openAction->setShortcut(QKeySequence("Ctrl+O"));
    saveAction->setShortcut(QKeySequence("Ctrl+S"));
    openCompareAction->setShortcut(QKeySequence("Ctrl+D"));
    quitAction->setShortcut(QKeySequence("Ctrl+Q"));

    // Edit menu includes undo stack actions and document mutation commands.
    auto* editMenu = menuBar()->addMenu("&Edit");
    auto* undoAction = undoStack_.createUndoAction(this, "Undo");
    auto* redoAction = undoStack_.createRedoAction(this, "Redo");
    undoAction->setShortcut(QKeySequence::Undo);
    redoAction->setShortcut(QKeySequence::Redo);
    editMenu->addAction(undoAction);
    editMenu->addAction(redoAction);
    editMenu->addSeparator();
    auto* addSettingAction = editMenu->addAction("Add Setting...");
    auto* deleteSettingAction = editMenu->addAction("Delete Selected Setting");
    auto* addSectionAction = editMenu->addAction("Add Section...");
    auto* deleteSectionAction = editMenu->addAction("Delete Section...");
    auto* focusSearchAction = editMenu->addAction("Focus Search");
    focusSearchAction->setShortcut(QKeySequence("Ctrl+F"));

    // Signals/slots wiring:
    // sender signal -> receiver slot/lambda
    // Qt automatically disconnects links when QObject receivers are destroyed.
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenFile);
    connect(saveAction, &QAction::triggered, this, &MainWindow::onSaveFile);
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::onSaveAsFile);
    connect(openCompareAction, &QAction::triggered, this, &MainWindow::onOpenCompareFile);
    connect(quitAction, &QAction::triggered, this, &QWidget::close);
    connect(addSettingAction, &QAction::triggered, this, &MainWindow::onAddSetting);
    connect(deleteSettingAction, &QAction::triggered, this, &MainWindow::onDeleteSelectedSetting);
    connect(addSectionAction, &QAction::triggered, this, &MainWindow::onAddSection);
    connect(deleteSectionAction, &QAction::triggered, this, &MainWindow::onDeleteSection);
    connect(focusSearchAction, &QAction::triggered, searchEdit_, [this] { searchEdit_->setFocus(); });

    // View menu exposes dock visibility and layout recovery actions.
    auto* viewMenu = menuBar()->addMenu("&View");
    viewMenu->addAction(filesDock_->toggleViewAction());
    viewMenu->addAction(diffMergeDock_->toggleViewAction());
    viewMenu->addAction(statusDock_->toggleViewAction());
    viewMenu->addSeparator();
    auto* showAllPanelsAction = viewMenu->addAction("Show All Panels");
    auto* redockPanelsAction = viewMenu->addAction("Redock All Panels");
    redockPanelsAction->setShortcut(QKeySequence("Ctrl+Shift+R"));

    connect(showAllPanelsAction, &QAction::triggered, this, &MainWindow::showAllPanels);
    connect(redockPanelsAction, &QAction::triggered, this, &MainWindow::redockAllPanels);

    auto* presetsMenu = menuBar()->addMenu("&Presets");
    // Preset actions are grouped by lifecycle:
    // create/update -> maintenance -> apply.
    auto* saveSectionPresetAction = presetsMenu->addAction("Save Section Preset...");
    auto* saveFilteredPresetAction = presetsMenu->addAction("Save Filtered Set Preset...");
    auto* addSelectedRowToPresetAction = presetsMenu->addAction("Add Selected Row To Preset...");
    auto* addSectionToPresetNoOverrideAction = presetsMenu->addAction("Add Section To Preset (No Override)...");
    presetsMenu->addSeparator();
    auto* renameSavedPresetAction = presetsMenu->addAction("Rename Saved Preset...");
    auto* deleteSavedPresetAction = presetsMenu->addAction("Delete Saved Preset...");
    presetsMenu->addSeparator();
    auto* applySavedPresetAction = presetsMenu->addAction("Apply Saved Preset...");

    connect(saveSectionPresetAction, &QAction::triggered, this, &MainWindow::onSaveSectionPreset);
    connect(saveFilteredPresetAction, &QAction::triggered, this, &MainWindow::onSaveFilteredPreset);
    connect(addSelectedRowToPresetAction, &QAction::triggered, this, &MainWindow::onAddSelectedRowToPreset);
    connect(addSectionToPresetNoOverrideAction, &QAction::triggered, this, &MainWindow::onAddSectionToPresetNoOverride);
    connect(renameSavedPresetAction, &QAction::triggered, this, &MainWindow::onRenameSavedPreset);
    connect(deleteSavedPresetAction, &QAction::triggered, this, &MainWindow::onDeleteSavedPreset);
    connect(applySavedPresetAction, &QAction::triggered, this, &MainWindow::onApplySavedPreset);
}

void MainWindow::setupConnections() {
    // Any domain change updates title/dirty marker and re-runs active filtering.
    connect(&document_, &IniDocument::changed, this, [this] {
        updateWindowTitle();
        filterProxyModel_.invalidate();
    });

    // Search controls all trigger the same filter refresh path.
    connect(searchEdit_, &QLineEdit::textChanged, this, &MainWindow::onSearchChanged);
    connect(caseSensitiveCheck_, &QCheckBox::toggled, this, &MainWindow::onSearchChanged);
    connect(regexCheck_, &QCheckBox::toggled, this, &MainWindow::onSearchChanged);

    // Proxy emits visible row count and regex validity for UI feedback.
    connect(&filterProxyModel_, &IniFilterProxyModel::filterApplied, this, [this](int visibleCount, bool regexValid) {
        const QString suffix = regexValid ? QString() : QStringLiteral(" (invalid regex)");
        searchCountLabel_->setText(QStringLiteral("Matches: %1%2").arg(visibleCount).arg(suffix));
    });

    // Async open result handler (runs on GUI thread after watcher signals finished()).
    connect(&openWatcher_, &QFutureWatcher<IniParseResult>::finished, this, [this] {
        const auto result = openWatcher_.result();
        if (!result.ok) {
            QMessageBox::critical(this, "Open failed", result.error);
            statusBar()->showMessage("Open failed", 3000);
            return;
        }
        document_.restore(result.snapshot);
        document_.setDirty(false);
        fileList_->addItem(result.snapshot.path);
        logMessage(QStringLiteral("Opened %1").arg(result.snapshot.path));
        statusBar()->showMessage("File opened", 2000);
    });

    // Async compare-open result handler.
    connect(&compareOpenWatcher_, &QFutureWatcher<IniParseResult>::finished, this, [this] {
        const auto result = compareOpenWatcher_.result();
        if (!result.ok) {
            QMessageBox::critical(this, "Compare open failed", result.error);
            return;
        }
        compareDocument_.restore(result.snapshot);
        compareDocument_.setDirty(false);
        fileList_->addItem(QStringLiteral("Compare: %1").arg(result.snapshot.path));
        runDiffAsync();
    });

    // Async diff result handler.
    connect(&diffWatcher_, &QFutureWatcher<DiffComputationResult>::finished, this, [this] {
        const auto result = diffWatcher_.result();
        diffTableModel_.setItems(result.items);
        diffSummaryLabel_->setText(QStringLiteral("Raw diff summary: +%1 / -%2 / ~%3")
                                       .arg(result.summary.addedLines)
                                       .arg(result.summary.removedLines)
                                       .arg(result.summary.changedLines));
        statusBar()->showMessage("Diff complete", 2500);
    });

    // Button lookups are done by objectName so setupUi and setupConnections remain decoupled.
    if (auto* button = findChild<QPushButton*>("loadCompareButton")) {
        connect(button, &QPushButton::clicked, this, &MainWindow::onOpenCompareFile);
    }
    if (auto* button = findChild<QPushButton*>("recomputeDiffButton")) {
        connect(button, &QPushButton::clicked, this, [this] { runDiffAsync(); });
    }
    if (auto* button = findChild<QPushButton*>("previewMergeButton")) {
        connect(button, &QPushButton::clicked, this, &MainWindow::onPrepareMergePreview);
    }
    if (auto* button = findChild<QPushButton*>("applyMergeButton")) {
        connect(button, &QPushButton::clicked, this, &MainWindow::onApplyMerge);
    }
    if (auto* button = findChild<QPushButton*>("selectAllMergeButton")) {
        connect(button, &QPushButton::clicked, this, [this] { mergePreviewModel_.selectAll(true); });
    }
    if (auto* button = findChild<QPushButton*>("selectChangedMergeButton")) {
        connect(button, &QPushButton::clicked, this, [this] { mergePreviewModel_.selectByStatus(SemanticDiffStatus::Changed); });
    }
    if (auto* button = findChild<QPushButton*>("selectAddedMergeButton")) {
        connect(button, &QPushButton::clicked, this, [this] { mergePreviewModel_.selectByStatus(SemanticDiffStatus::Added); });
    }
    if (auto* button = findChild<QPushButton*>("clearMergeSelectionButton")) {
        connect(button, &QPushButton::clicked, this, [this] { mergePreviewModel_.selectAll(false); });
    }
}

void MainWindow::applyDarkTheme() {
    // Acquire global QApplication instance to apply app-wide style and palette.
    auto* app = qobject_cast<QApplication*>(QCoreApplication::instance());
    if (!app) {
        return;
    }
    app->setStyle(QStyleFactory::create("Fusion"));

    // Palette controls base colors used by Fusion style.
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(30, 30, 30));
    palette.setColor(QPalette::WindowText, QColor(212, 212, 212));
    palette.setColor(QPalette::Base, QColor(30, 30, 30));
    palette.setColor(QPalette::AlternateBase, QColor(37, 37, 38));
    palette.setColor(QPalette::ToolTipBase, QColor(45, 45, 45));
    palette.setColor(QPalette::ToolTipText, QColor(212, 212, 212));
    palette.setColor(QPalette::Text, QColor(212, 212, 212));
    palette.setColor(QPalette::Button, QColor(45, 45, 45));
    palette.setColor(QPalette::ButtonText, QColor(212, 212, 212));
    palette.setColor(QPalette::BrightText, QColor(255, 255, 255));
    palette.setColor(QPalette::Highlight, QColor(9, 71, 113));
    palette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
    palette.setColor(QPalette::PlaceholderText, QColor(133, 133, 133));
    app->setPalette(palette);

    // Style sheet applies widget-specific overrides.
    setStyleSheet(QString(
        "QMainWindow { background-color: #1e1e1e; border: 1px solid #3c3c3c; }"
        "QMainWindow::separator { background: rgba(220, 220, 220, 0.55); width: 1px; height: 1px; }"
        "QToolBar { background-color: #2d2d30; border: none; spacing: 8px; padding: 8px; }"
        "QStatusBar { background: #007acc; color: #ffffff; border: none; }"
        "QDockWidget { color: #d4d4d4; font-weight: 600; background: #252526; border: none; padding-top: 0px; }"
        "QDockWidget::title { background: #2d2d30; color: #cccccc; padding: %2px 10px; margin: 0px; border: none; height: %1px; min-height: %1px; max-height: %1px; }"
        "QDockWidget::close-button, QDockWidget::float-button { background: transparent; border: none; image: none; }"
        "QDockWidget::close-button:hover, QDockWidget::float-button:hover { background: transparent; }"
        "QMenuBar { background: #2d2d30; color: #d4d4d4; border: none; }"
        "QMenuBar::item:selected { background: #3e3e42; border: none; }"
        "QMenu { background: #252526; border: 1px solid #3c3c3c; color: #d4d4d4; padding: 4px; }"
        "QMenu::item:selected { background: #094771; color: #ffffff; }"
        "QTabWidget::pane { border: 1px solid #3c3c3c; top: -1px; }"
        "QTabBar::tab { background: #2d2d30; color: #969696; padding: 8px 14px; margin-right: 2px; border: none; }"
        "QTabBar::tab:selected { background: #1e1e1e; color: #ffffff; }"
        "QPushButton { background: #0e639c; color: #ffffff; border: none; padding: 7px 12px; }"
        "QPushButton:hover { background: #1177bb; }"
        "QPushButton:pressed { background: #0b4f7d; }"
        "QLineEdit, QComboBox { background: #3c3c3c; color: #d4d4d4; border: 1px solid #3c3c3c; padding: 6px 8px; selection-background-color: #094771; }"
        "QLineEdit:focus, QComboBox:focus { border: 1px solid #007acc; }"
        "QCheckBox { color: #d4d4d4; spacing: 6px; }"
        "QCheckBox::indicator { width: 15px; height: 15px; border: 1px solid #5a5a5a; background: #1e1e1e; }"
        "QCheckBox::indicator:checked { background: #007acc; border: 1px solid #007acc; }"
        "QListWidget, QTextEdit, QTableView { background: #1e1e1e; color: #d4d4d4; border: none; gridline-color: #2d2d30; selection-background-color: #094771; selection-color: #ffffff; }"
        "QTableView QHeaderView::section:horizontal { background: #2d2d30; color: #d4d4d4; border: none; border-bottom: 1px solid #3c3c3c; padding: 0px 8px; min-height: %1px; max-height: %1px; font-weight: 600; }"
        "QTableCornerButton::section { background: #2d2d30; border: none; border-bottom: 1px solid #3c3c3c; }"
        "QScrollBar:vertical { background: #252526; width: 12px; margin: 0px; border: none; }"
        "QScrollBar::handle:vertical { background: #424242; min-height: 24px; border: none; }"
        "QScrollBar::handle:vertical:hover { background: #4f4f4f; }"
        "QScrollBar:horizontal { background: #252526; height: 12px; margin: 0px; border: none; }"
        "QScrollBar::handle:horizontal { background: #424242; min-width: 24px; border: none; }"
        "QToolTip { background: #252526; color: #d4d4d4; border: 1px solid #3c3c3c; }")
                      .arg(kMainHeaderHeight)
                      .arg(kDockTitleVerticalPadding));
}

// ----- Layout helpers -----

void MainWindow::showAllPanels() {
    filesDock_->show();
    diffMergeDock_->show();
    statusDock_->show();
}

void MainWindow::redockAllPanels() {
    // Return docks to deterministic default positions.
    showAllPanels();

    filesDock_->setFloating(false);
    diffMergeDock_->setFloating(false);
    statusDock_->setFloating(false);

    addDockWidget(Qt::LeftDockWidgetArea, filesDock_);
    addDockWidget(Qt::RightDockWidgetArea, diffMergeDock_);
    addDockWidget(Qt::BottomDockWidgetArea, statusDock_);
}

void MainWindow::updateWindowTitle() {
    // Title reflects current file path and dirty marker.
    const QString fileName = document_.path().isEmpty() ? QStringLiteral("Untitled") : document_.path();
    const QString dirtyMarker = document_.isDirty() ? QStringLiteral("*") : QString();
    setWindowTitle(QStringLiteral("INIX - %1%2").arg(fileName, dirtyMarker));
}

void MainWindow::logMessage(const QString& message) { logOutput_->append(message); }

// ----- Search and async operations -----

SearchOptions MainWindow::currentSearchOptions() const {
    return SearchOptions{
        .query = searchEdit_->text(),
        .caseSensitive = caseSensitiveCheck_->isChecked(),
        .regex = regexCheck_->isChecked(),
    };
}

void MainWindow::runOpenAsync(const QString& path) {
    // QtConcurrent::run executes parser on worker thread and returns QFuture.
    statusBar()->showMessage("Opening file...");
    openWatcher_.setFuture(QtConcurrent::run([this, path] { return parser_.parseFile(path); }));
}

void MainWindow::runCompareOpenAsync(const QString& path) {
    statusBar()->showMessage("Opening compare file...");
    compareOpenWatcher_.setFuture(QtConcurrent::run([this, path] { return parser_.parseFile(path); }));
}

void MainWindow::runDiffAsync() {
    if (document_.isEmpty() || compareDocument_.isEmpty()) {
        statusBar()->showMessage("Load both target and compare files first", 2500);
        return;
    }
    statusBar()->showMessage("Computing diff...");
    // Compute both text summary and semantic diff off the UI thread.
    diffWatcher_.setFuture(QtConcurrent::run([this] {
        DiffComputationResult output;
        output.summary = diffService_.buildTextSummary(document_, compareDocument_);
        output.items = diffService_.buildSemanticDiff(document_, compareDocument_, true);
        return output;
    }));
}

QString MainWindow::savedSnippetsPath() const {
    // Presets are persisted beside the executable as requested.
    // This keeps data portable with the app folder.
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("saved_snippets.json"));
}

bool MainWindow::loadSavedSnippets(QJsonObject& snippets, QString* error) const {
    // JSON schema:
    // {
    //   "Preset Name": "[Section]\nkey=value\n..."
    // }
    //
    // Each property value is the raw INI snippet string.
    snippets = QJsonObject{};

    QFile file(savedSnippetsPath());
    if (!file.exists()) {
        return true;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = QStringLiteral("Unable to open preset file: %1").arg(file.fileName());
        }
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (error) {
            *error = QStringLiteral("Preset file is not valid JSON: %1").arg(parseError.errorString());
        }
        return false;
    }
    if (!doc.isObject()) {
        if (error) {
            *error = QStringLiteral("Preset file must contain a JSON object.");
        }
        return false;
    }

    snippets = doc.object();
    return true;
}

bool MainWindow::writeSavedSnippets(const QJsonObject& snippets, QString* error) const {
    const QString filePath = savedSnippetsPath();
    const QFileInfo fileInfo(filePath);
    QDir dir = fileInfo.dir();
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        if (error) {
            *error = QStringLiteral("Unable to create preset directory: %1").arg(dir.path());
        }
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) {
            *error = QStringLiteral("Unable to write preset file: %1").arg(filePath);
        }
        return false;
    }

    // Use indented output for human readability and easier manual inspection.
    const QJsonDocument doc(snippets);
    if (file.write(doc.toJson(QJsonDocument::Indented)) < 0) {
        if (error) {
            *error = QStringLiteral("Failed to write preset file: %1").arg(filePath);
        }
        return false;
    }
    return true;
}

QString MainWindow::buildSectionSnippet(const QString& section) const {
    // Export only key/value lines from the requested section.
    // A section header is included so snippet can be parsed/applied later.
    const QString normalizedSection = section.trimmed();
    if (normalizedSection.isEmpty()) {
        return {};
    }

    QStringList lines;
    lines.push_back(QStringLiteral("[%1]").arg(normalizedSection));

    for (const auto& line : document_.lines()) {
        if (line.type != IniLineType::KeyValue) {
            continue;
        }
        if (line.section.compare(normalizedSection, Qt::CaseInsensitive) != 0) {
            continue;
        }
        lines.push_back(QStringLiteral("%1=%2").arg(line.key, line.value));
    }

    if (lines.size() == 1) {
        return {};
    }
    return lines.join(QStringLiteral("\n"));
}

QString MainWindow::buildFilteredSetSnippet() const {
    // Export currently visible rows in proxy order (after search/filter).
    // This allows users to snapshot arbitrary sets quickly.
    struct SectionBucket {
        QString sectionName;
        QStringList values;
    };

    QVector<SectionBucket> buckets;
    QHash<QString, int> sectionIndex;

    const QAbstractItemModel* proxyModel = settingsTable_->model();
    if (!proxyModel) {
        return {};
    }

    for (int row = 0; row < proxyModel->rowCount(); ++row) {
        const QString section = proxyModel->data(proxyModel->index(row, 0), Qt::DisplayRole).toString().trimmed();
        const QString key = proxyModel->data(proxyModel->index(row, 1), Qt::DisplayRole).toString();
        const QString value = proxyModel->data(proxyModel->index(row, 2), Qt::DisplayRole).toString();
        if (section.isEmpty() || key.isEmpty()) {
            continue;
        }

        const QString sectionToken = section.toLower();
        int bucketIndex = sectionIndex.value(sectionToken, -1);
        if (bucketIndex < 0) {
            bucketIndex = buckets.size();
            sectionIndex.insert(sectionToken, bucketIndex);
            buckets.push_back(SectionBucket{.sectionName = section, .values = {}});
        }
        buckets[bucketIndex].values.push_back(QStringLiteral("%1=%2").arg(key, value));
    }

    QStringList lines;
    for (int i = 0; i < buckets.size(); ++i) {
        const auto& bucket = buckets[i];
        lines.push_back(QStringLiteral("[%1]").arg(bucket.sectionName));
        lines.append(bucket.values);
        if (i + 1 < buckets.size()) {
            lines.push_back(QString());
        }
    }

    return lines.join(QStringLiteral("\n"));
}

QString MainWindow::buildSelectedRowSnippet() const {
    const auto proxyIndex = settingsTable_->currentIndex();
    if (!proxyIndex.isValid()) {
        return {};
    }

    const QAbstractItemModel* model = settingsTable_->model();
    if (!model) {
        return {};
    }

    const QString section = model->data(model->index(proxyIndex.row(), 0), Qt::DisplayRole).toString().trimmed();
    const QString key = model->data(model->index(proxyIndex.row(), 1), Qt::DisplayRole).toString().trimmed();
    const QString value = model->data(model->index(proxyIndex.row(), 2), Qt::DisplayRole).toString();
    if (section.isEmpty() || key.isEmpty()) {
        return {};
    }

    return QStringLiteral("[%1]\n%2=%3").arg(section, key, value);
}

bool MainWindow::saveSnippetByName(const QString& name, const QString& snippet, QString& error) {
    const QString trimmedName = name.trimmed();
    if (trimmedName.isEmpty()) {
        error = QStringLiteral("Preset name cannot be empty.");
        return false;
    }
    if (snippet.trimmed().isEmpty()) {
        error = QStringLiteral("Preset content is empty.");
        return false;
    }

    QJsonObject snippets;
    if (!loadSavedSnippets(snippets, &error)) {
        return false;
    }

    // Explicit overwrite confirmation protects against accidental replacement.
    if (snippets.contains(trimmedName)) {
        const auto overwrite = QMessageBox::question(this,
                                                     "Overwrite preset",
                                                     QStringLiteral("Preset \"%1\" already exists. Overwrite it?").arg(trimmedName));
        if (overwrite != QMessageBox::Yes) {
            error = QStringLiteral("Preset save cancelled.");
            return false;
        }
    }

    snippets.insert(trimmedName, snippet);
    return writeSavedSnippets(snippets, &error);
}

bool MainWindow::selectSavedPresetName(QString& presetName, QString& error) const {
    presetName.clear();

    QJsonObject snippets;
    if (!loadSavedSnippets(snippets, &error)) {
        return false;
    }
    if (snippets.isEmpty()) {
        error = QStringLiteral("No saved presets were found.");
        return false;
    }

    QStringList names = snippets.keys();
    std::sort(names.begin(), names.end(), [](const QString& a, const QString& b) {
        return a.compare(b, Qt::CaseInsensitive) < 0;
    });

    bool accepted = false;
    // QInputDialog static API expects QWidget* parent. This helper is const
    // because it does not mutate logical MainWindow state.
    const QString selected =
        QInputDialog::getItem(const_cast<MainWindow*>(this), "Choose Preset", "Preset:", names, 0, false, &accepted);
    if (!accepted || selected.isEmpty()) {
        error = QStringLiteral("Cancelled.");
        return false;
    }

    presetName = selected;
    return true;
}

bool MainWindow::mergeSnippetIntoPreset(const QString& presetName,
                                        const QString& snippet,
                                        bool overwriteExistingKeys,
                                        QString& error) {
    // Merge strategy:
    // - Read existing preset snippet into normalized entries.
    // - Read incoming snippet entries.
    // - Match by case-insensitive (section,key).
    // - Add new entries always.
    // - Update existing values only when overwriteExistingKeys == true.
    const QString normalizedName = presetName.trimmed();
    if (normalizedName.isEmpty()) {
        error = QStringLiteral("Preset name cannot be empty.");
        return false;
    }

    QJsonObject snippets;
    if (!loadSavedSnippets(snippets, &error)) {
        return false;
    }
    if (!snippets.contains(normalizedName)) {
        error = QStringLiteral("Preset \"%1\" does not exist.").arg(normalizedName);
        return false;
    }

    const QVector<SnippetEntry> existingEntries = parseSnippetEntries(snippets.value(normalizedName).toString());
    const QVector<SnippetEntry> incomingEntries = parseSnippetEntries(snippet);
    if (incomingEntries.isEmpty()) {
        error = QStringLiteral("No valid key/value rows were found to add.");
        return false;
    }

    QVector<SnippetEntry> merged = existingEntries;
    QHash<QString, int> entryIndex;
    for (int i = 0; i < merged.size(); ++i) {
        entryIndex.insert(entryToken(merged[i].section, merged[i].key), i);
    }

    for (const auto& item : incomingEntries) {
        const QString token = entryToken(item.section, item.key);
        const int idx = entryIndex.value(token, -1);
        if (idx < 0) {
            entryIndex.insert(token, merged.size());
            merged.push_back(item);
            continue;
        }
        if (overwriteExistingKeys) {
            merged[idx].value = item.value;
        }
    }

    snippets.insert(normalizedName, buildSnippetFromEntries(merged));
    return writeSavedSnippets(snippets, &error);
}

int MainWindow::applySnippet(const QString& snippet, QString& error) {
    // Apply strategy mirrors preset parsing behavior:
    // - parse section headers + key/value rows
    // - ignore comments/blank/malformed lines
    // - upsert each valid row into document_
    //
    // Upsert semantics:
    // - existing key in section -> value updated
    // - missing key -> inserted
    int applied = 0;
    QString currentSection;
    const QStringList lines = snippet.split(QRegularExpression("\r\n|\n|\r"), Qt::KeepEmptyParts);
    for (const QString& rawLine : lines) {
        const QString trimmed = rawLine.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith(';') || trimmed.startsWith('#')) {
            continue;
        }

        if (trimmed.startsWith('[') && trimmed.endsWith(']') && trimmed.size() > 2) {
            currentSection = trimmed.mid(1, trimmed.size() - 2).trimmed();
            continue;
        }

        const int equalsPos = rawLine.indexOf('=');
        if (equalsPos <= 0 || currentSection.isEmpty()) {
            continue;
        }

        const QString key = rawLine.left(equalsPos).trimmed();
        const QString value = rawLine.mid(equalsPos + 1);
        if (key.isEmpty()) {
            continue;
        }

        if (document_.upsertSetting(currentSection, key, value)) {
            ++applied;
        }
    }

    if (applied == 0) {
        error = QStringLiteral("Preset contains no applicable key/value entries.");
    }
    return applied;
}

// ----- Slots: file/document actions -----

void MainWindow::onOpenFile() {
    const QString path = QFileDialog::getOpenFileName(this, "Open INI", QString(), "INI Files (*.ini);;All Files (*.*)");
    if (path.isEmpty()) {
        return;
    }
    runOpenAsync(path);
}

void MainWindow::onSaveFile() {
    // Delegate to Save As if path is unknown.
    if (document_.path().isEmpty()) {
        onSaveAsFile();
        return;
    }
    QString error;
    if (!serializer_.writeToFile(document_, document_.path(), error)) {
        QMessageBox::critical(this, "Save failed", error);
        return;
    }
    document_.setDirty(false);
    statusBar()->showMessage("Saved", 1500);
}

void MainWindow::onSaveAsFile() {
    const QString path = QFileDialog::getSaveFileName(this, "Save INI As", document_.path(), "INI Files (*.ini)");
    if (path.isEmpty()) {
        return;
    }
    QString error;
    if (!serializer_.writeToFile(document_, path, error)) {
        QMessageBox::critical(this, "Save As failed", error);
        return;
    }
    document_.setPath(path);
    document_.setDirty(false);
    statusBar()->showMessage("Saved", 1500);
}

void MainWindow::onAddSetting() {
    const QString section = QInputDialog::getText(this, "Section", "Section:");
    if (section.isEmpty()) {
        return;
    }
    const QString key = QInputDialog::getText(this, "Key", "Key:");
    if (key.isEmpty()) {
        return;
    }
    const QString value = QInputDialog::getText(this, "Value", "Value:");
    settingsModel_.addSetting(section, key, value);
}

void MainWindow::onDeleteSelectedSetting() {
    // Selection lives in proxy coordinates; convert back to source model row.
    const auto proxyIndex = settingsTable_->currentIndex();
    if (!proxyIndex.isValid()) {
        return;
    }
    const auto sourceIndex = filterProxyModel_.mapToSource(proxyIndex);
    settingsModel_.deleteSettingAtRow(sourceIndex.row());
}

void MainWindow::onAddSection() {
    const QString section = QInputDialog::getText(this, "Add Section", "Section name:");
    if (section.isEmpty()) {
        return;
    }
    settingsModel_.addSection(section);
}

void MainWindow::onDeleteSection() {
    const QString section = QInputDialog::getText(this, "Delete Section", "Section name:");
    if (section.isEmpty()) {
        return;
    }
    settingsModel_.deleteSection(section);
}

void MainWindow::onOpenCompareFile() {
    const QString path = QFileDialog::getOpenFileName(this, "Open Compare INI", QString(), "INI Files (*.ini)");
    if (path.isEmpty()) {
        return;
    }
    runCompareOpenAsync(path);
}

// ----- Slots: diff and merge -----

void MainWindow::onPrepareMergePreview() {
    // If diff is stale/missing, trigger recompute first.
    if (diffTableModel_.items().isEmpty()) {
        runDiffAsync();
        return;
    }
    mergePreviewModel_.setItems(mergeService_.buildPreview(diffTableModel_.items()));
}

void MainWindow::onApplyMerge() {
    auto previewItems = mergePreviewModel_.items();
    if (previewItems.isEmpty()) {
        QMessageBox::information(this, "Merge", "Prepare a merge preview first.");
        return;
    }
    // Translate combo index to explicit enum policy.
    MergeConflictPolicy policy = MergeConflictPolicy::ReplaceTarget;
    if (conflictPolicyCombo_->currentIndex() == 1) {
        policy = MergeConflictPolicy::KeepTarget;
    } else if (conflictPolicyCombo_->currentIndex() == 2) {
        policy = MergeConflictPolicy::PromptPerConflict;
    }

    // Snapshot full state so operation is undoable in one command.
    const auto before = document_.snapshot();
    const auto summary = mergeService_.applyPreview(
        document_, previewItems, policy, [this](const MergePreviewItem& item) {
            const auto result = QMessageBox::question(
                this,
                "Merge conflict",
                QStringLiteral("Replace [%1] %2?\nTarget: %3\nSource: %4")
                    .arg(item.section, item.key, item.targetValue, item.sourceValue));
            return result == QMessageBox::Yes;
        });
    const auto after = document_.snapshot();
    undoStack_.push(new MergeApplyCommand(&document_, before, after));

    QMessageBox::information(this,
                             "Merge complete",
                             QStringLiteral("Added: %1\nReplaced: %2\nKept: %3\nSkipped: %4")
                                 .arg(summary.added)
                                 .arg(summary.replaced)
                                 .arg(summary.kept)
                                 .arg(summary.skipped));
}

void MainWindow::onSearchChanged() { filterProxyModel_.setSearchOptions(currentSearchOptions()); }

void MainWindow::onSaveSectionPreset() {
    // Saves one section from the current in-memory document into JSON presets.
    QString defaultSection;
    const auto current = settingsTable_->currentIndex();
    if (current.isValid()) {
        defaultSection = settingsTable_->model()->data(settingsTable_->model()->index(current.row(), 0), Qt::DisplayRole).toString();
    }

    const QString section = QInputDialog::getText(this, "Save Section Preset", "Section name:", QLineEdit::Normal, defaultSection);
    if (section.trimmed().isEmpty()) {
        return;
    }

    const QString snippet = buildSectionSnippet(section);
    if (snippet.isEmpty()) {
        QMessageBox::information(this, "Save Section Preset", "No key/value entries were found in that section.");
        return;
    }

    const QString presetName = QInputDialog::getText(this, "Save Section Preset", "Preset name:");
    if (presetName.trimmed().isEmpty()) {
        return;
    }

    QString error;
    if (!saveSnippetByName(presetName, snippet, error)) {
        if (error != QStringLiteral("Preset save cancelled.")) {
            QMessageBox::critical(this, "Save Section Preset", error);
        }
        return;
    }

    logMessage(QStringLiteral("Saved section preset \"%1\"").arg(presetName.trimmed()));
    statusBar()->showMessage("Section preset saved", 2500);
}

void MainWindow::onSaveFilteredPreset() {
    // Saves the exact visible subset from the filtered proxy table.
    const QString snippet = buildFilteredSetSnippet();
    if (snippet.trimmed().isEmpty()) {
        QMessageBox::information(this, "Save Filtered Set Preset", "No visible values are available to save.");
        return;
    }

    const QString presetName = QInputDialog::getText(this, "Save Filtered Set Preset", "Preset name:");
    if (presetName.trimmed().isEmpty()) {
        return;
    }

    QString error;
    if (!saveSnippetByName(presetName, snippet, error)) {
        if (error != QStringLiteral("Preset save cancelled.")) {
            QMessageBox::critical(this, "Save Filtered Set Preset", error);
        }
        return;
    }

    logMessage(QStringLiteral("Saved filtered preset \"%1\"").arg(presetName.trimmed()));
    statusBar()->showMessage("Filtered preset saved", 2500);
}

void MainWindow::onApplySavedPreset() {
    // User picks a preset by name, then snippet is parsed and upserted.
    QString error;
    QString selectedName;
    if (!selectSavedPresetName(selectedName, error)) {
        if (error != QStringLiteral("Cancelled.")) {
            QMessageBox::information(this, "Apply Saved Preset", error);
        }
        return;
    }

    QJsonObject snippets;
    if (!loadSavedSnippets(snippets, &error)) {
        QMessageBox::critical(this, "Apply Saved Preset", error);
        return;
    }

    const QString snippet = snippets.value(selectedName).toString();
    const int appliedCount = applySnippet(snippet, error);
    if (appliedCount <= 0) {
        QMessageBox::critical(this, "Apply Saved Preset", error);
        return;
    }

    QMessageBox::information(this,
                             "Apply Saved Preset",
                             QStringLiteral("Applied preset \"%1\".\nUpdated entries: %2")
                                 .arg(selectedName)
                                 .arg(appliedCount));
    logMessage(QStringLiteral("Applied preset \"%1\" (%2 entries)").arg(selectedName).arg(appliedCount));
    statusBar()->showMessage(QStringLiteral("Applied preset: %1").arg(selectedName), 2500);
}

void MainWindow::onRenameSavedPreset() {
    // Renames JSON key while preserving snippet payload exactly.
    QString error;
    QString oldName;
    if (!selectSavedPresetName(oldName, error)) {
        if (error != QStringLiteral("Cancelled.")) {
            QMessageBox::information(this, "Rename Saved Preset", error);
        }
        return;
    }

    const QString newName =
        QInputDialog::getText(this, "Rename Saved Preset", "New preset name:", QLineEdit::Normal, oldName);
    if (newName.trimmed().isEmpty() || newName.trimmed() == oldName) {
        return;
    }

    QJsonObject snippets;
    if (!loadSavedSnippets(snippets, &error)) {
        QMessageBox::critical(this, "Rename Saved Preset", error);
        return;
    }
    if (snippets.contains(newName.trimmed())) {
        QMessageBox::critical(this, "Rename Saved Preset", "A preset with that name already exists.");
        return;
    }

    const QString content = snippets.value(oldName).toString();
    snippets.remove(oldName);
    snippets.insert(newName.trimmed(), content);
    if (!writeSavedSnippets(snippets, &error)) {
        QMessageBox::critical(this, "Rename Saved Preset", error);
        return;
    }

    logMessage(QStringLiteral("Renamed preset \"%1\" to \"%2\"").arg(oldName, newName.trimmed()));
    statusBar()->showMessage("Preset renamed", 2500);
}

void MainWindow::onDeleteSavedPreset() {
    // Removes one preset entry from the JSON dictionary.
    QString error;
    QString presetName;
    if (!selectSavedPresetName(presetName, error)) {
        if (error != QStringLiteral("Cancelled.")) {
            QMessageBox::information(this, "Delete Saved Preset", error);
        }
        return;
    }

    const auto confirm =
        QMessageBox::question(this, "Delete Saved Preset", QStringLiteral("Delete preset \"%1\"?").arg(presetName));
    if (confirm != QMessageBox::Yes) {
        return;
    }

    QJsonObject snippets;
    if (!loadSavedSnippets(snippets, &error)) {
        QMessageBox::critical(this, "Delete Saved Preset", error);
        return;
    }
    snippets.remove(presetName);
    if (!writeSavedSnippets(snippets, &error)) {
        QMessageBox::critical(this, "Delete Saved Preset", error);
        return;
    }

    logMessage(QStringLiteral("Deleted preset \"%1\"").arg(presetName));
    statusBar()->showMessage("Preset deleted", 2500);
}

void MainWindow::onAddSelectedRowToPreset() {
    // Adds the currently selected row into an existing preset.
    // Existing entry for same (section,key) is updated.
    const QString rowSnippet = buildSelectedRowSnippet();
    if (rowSnippet.isEmpty()) {
        QMessageBox::information(this, "Add Selected Row To Preset", "Select one settings row first.");
        return;
    }

    QString error;
    QString presetName;
    if (!selectSavedPresetName(presetName, error)) {
        if (error != QStringLiteral("Cancelled.")) {
            QMessageBox::information(this, "Add Selected Row To Preset", error);
        }
        return;
    }

    if (!mergeSnippetIntoPreset(presetName, rowSnippet, true, error)) {
        QMessageBox::critical(this, "Add Selected Row To Preset", error);
        return;
    }

    logMessage(QStringLiteral("Added selected row to preset \"%1\"").arg(presetName));
    statusBar()->showMessage("Row added to preset", 2500);
}

void MainWindow::onAddSectionToPresetNoOverride() {
    // Merges a section into an existing preset while preserving any existing
    // values already present in that preset for matching keys.
    QString defaultSection;
    const auto current = settingsTable_->currentIndex();
    if (current.isValid()) {
        defaultSection = settingsTable_->model()->data(settingsTable_->model()->index(current.row(), 0), Qt::DisplayRole).toString();
    }

    const QString section =
        QInputDialog::getText(this, "Add Section To Preset (No Override)", "Section name:", QLineEdit::Normal, defaultSection);
    if (section.trimmed().isEmpty()) {
        return;
    }

    const QString sectionSnippet = buildSectionSnippet(section);
    if (sectionSnippet.isEmpty()) {
        QMessageBox::information(this, "Add Section To Preset (No Override)", "No key/value entries were found in that section.");
        return;
    }

    QString error;
    QString presetName;
    if (!selectSavedPresetName(presetName, error)) {
        if (error != QStringLiteral("Cancelled.")) {
            QMessageBox::information(this, "Add Section To Preset (No Override)", error);
        }
        return;
    }

    if (!mergeSnippetIntoPreset(presetName, sectionSnippet, false, error)) {
        QMessageBox::critical(this, "Add Section To Preset (No Override)", error);
        return;
    }

    logMessage(QStringLiteral("Added section \"%1\" to preset \"%2\" without overriding existing keys")
                   .arg(section.trimmed(), presetName));
    statusBar()->showMessage("Section merged into preset", 2500);
}
