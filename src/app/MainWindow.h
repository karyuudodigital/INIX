/*
    File: app/MainWindow.h
    Purpose:
      - Declares the top-level Qt window for INIX.
      - Coordinates UI widgets with domain state, asynchronous services, and table models.

    How it fits in the codebase:
      - Owns one editable target IniDocument and one compare IniDocument.
      - Uses services (parse/serialize/diff/merge/search) to transform data.
      - Binds data to Qt views through model classes under src/ui/models.
      - Acts as the "composition root" for the app runtime graph.
*/

#pragma once

#include "domain/IniDocument.h"
#include "services/IniDiffService.h"
#include "services/IniMergeService.h"
#include "services/IniParser.h"
#include "services/IniSerializer.h"
#include "ui/models/DiffTableModel.h"
#include "ui/models/IniFilterProxyModel.h"
#include "ui/models/IniSettingsTableModel.h"
#include "ui/models/MergePreviewTableModel.h"

#include <QFutureWatcher>
#include <QMainWindow>
#include <QJsonObject>
#include <QUndoStack>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QTableView;
class QTextEdit;
class QDockWidget;

// MainWindow is the UI orchestrator. It does not parse or diff directly;
// instead it delegates to dedicated services and updates models/views.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    // parent is optional in Qt. Null means this is a top-level native window.
    explicit MainWindow(QWidget* parent = nullptr);

private:
    // Small value type returned by the async diff computation.
    struct DiffComputationResult {
        TextDiffSummary summary;
        QVector<SemanticDiffItem> items;
    };

    // UI construction
    void setupUi();
    void setupMenus();
    void setupConnections();
    void applyDarkTheme();

    // Panel/window state helpers
    void showAllPanels();
    void redockAllPanels();
    void updateWindowTitle();
    void logMessage(const QString& message);

    // Search + async helpers
    SearchOptions currentSearchOptions() const;
    void runOpenAsync(const QString& path);
    void runCompareOpenAsync(const QString& path);
    void runDiffAsync();
    QString savedSnippetsPath() const;
    bool loadSavedSnippets(QJsonObject& snippets, QString* error = nullptr) const;
    bool writeSavedSnippets(const QJsonObject& snippets, QString* error = nullptr) const;
    QString buildSectionSnippet(const QString& section) const;
    QString buildFilteredSetSnippet() const;
    QString buildSelectedRowSnippet() const;
    bool saveSnippetByName(const QString& name, const QString& snippet, QString& error);
    bool selectSavedPresetName(QString& presetName, QString& error) const;
    bool mergeSnippetIntoPreset(const QString& presetName,
                                const QString& snippet,
                                bool overwriteExistingKeys,
                                QString& error);
    int applySnippet(const QString& snippet, QString& error);

private slots:
    // User-triggered actions exposed to menus/buttons.
    void onOpenFile();
    void onSaveFile();
    void onSaveAsFile();
    void onAddSetting();
    void onDeleteSelectedSetting();
    void onAddSection();
    void onDeleteSection();
    void onOpenCompareFile();
    void onPrepareMergePreview();
    void onApplyMerge();
    void onSearchChanged();
    void onSaveSectionPreset();
    void onSaveFilteredPreset();
    void onApplySavedPreset();
    void onRenameSavedPreset();
    void onDeleteSavedPreset();
    void onAddSelectedRowToPreset();
    void onAddSectionToPresetNoOverride();

private:
    // Domain state
    IniDocument document_;
    IniDocument compareDocument_;

    // Stateless services
    IniParser parser_;
    IniSerializer serializer_;
    IniDiffService diffService_;
    IniMergeService mergeService_;

    // Qt models consumed by views
    IniSettingsTableModel settingsModel_;
    IniFilterProxyModel filterProxyModel_;
    DiffTableModel diffTableModel_;
    MergePreviewTableModel mergePreviewModel_;

    // Watchers for background work launched by QtConcurrent::run
    QFutureWatcher<IniParseResult> openWatcher_;
    QFutureWatcher<IniParseResult> compareOpenWatcher_;
    QFutureWatcher<DiffComputationResult> diffWatcher_;

    // Undo stack for merge apply snapshots
    QUndoStack undoStack_;

    // Core widgets owned by MainWindow through QObject parent-child hierarchy
    QTableView* settingsTable_ = nullptr;
    QTableView* diffTable_ = nullptr;
    QTableView* mergeTable_ = nullptr;
    QDockWidget* filesDock_ = nullptr;
    QDockWidget* diffMergeDock_ = nullptr;
    QDockWidget* statusDock_ = nullptr;
    QListWidget* fileList_ = nullptr;
    QTextEdit* logOutput_ = nullptr;
    QLineEdit* searchEdit_ = nullptr;
    QCheckBox* caseSensitiveCheck_ = nullptr;
    QCheckBox* regexCheck_ = nullptr;
    QLabel* searchCountLabel_ = nullptr;
    QLabel* diffSummaryLabel_ = nullptr;
    QComboBox* conflictPolicyCombo_ = nullptr;
};
