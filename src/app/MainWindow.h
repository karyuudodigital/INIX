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
#include <QUndoStack>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QTableView;
class QTextEdit;
class QDockWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    struct DiffComputationResult {
        TextDiffSummary summary;
        QVector<SemanticDiffItem> items;
    };

    void setupUi();
    void setupMenus();
    void setupConnections();
    void applyDarkTheme();
    void showAllPanels();
    void redockAllPanels();
    void updateWindowTitle();
    void logMessage(const QString& message);
    SearchOptions currentSearchOptions() const;
    void runOpenAsync(const QString& path);
    void runCompareOpenAsync(const QString& path);
    void runDiffAsync();

private slots:
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

private:
    IniDocument document_;
    IniDocument compareDocument_;

    IniParser parser_;
    IniSerializer serializer_;
    IniDiffService diffService_;
    IniMergeService mergeService_;

    IniSettingsTableModel settingsModel_;
    IniFilterProxyModel filterProxyModel_;
    DiffTableModel diffTableModel_;
    MergePreviewTableModel mergePreviewModel_;

    QFutureWatcher<IniParseResult> openWatcher_;
    QFutureWatcher<IniParseResult> compareOpenWatcher_;
    QFutureWatcher<DiffComputationResult> diffWatcher_;

    QUndoStack undoStack_;

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
