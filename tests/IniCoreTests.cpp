#include "domain/IniDocument.h"
#include "services/IniDiffService.h"
#include "services/IniMergeService.h"
#include "services/IniParser.h"
#include "services/IniSerializer.h"

#include <QFile>
#include <QtTest/QtTest>

class IniCoreTests : public QObject {
    Q_OBJECT

private slots:
    void parser_handles_sections_duplicates_and_malformed();
    void serializer_round_trip_preserves_structure();
    void semantic_diff_detects_added_removed_changed_unchanged();
    void merge_policies_apply_as_expected();
    void integration_diff_and_merge_from_sample_files();
};

void IniCoreTests::parser_handles_sections_duplicates_and_malformed() {
    const QByteArray text =
        "[Display]\n"
        "fGamma=1.0000\n"
        "fGamma=1.1000\n"
        "; comment\n"
        "bad_line_without_equals\n"
        "# hash comment\n";

    IniParser parser;
    const auto result = parser.parseText(text);

    QVERIFY(result.ok);
    QCOMPARE(result.snapshot.lines.size(), 7);
    QCOMPARE(result.snapshot.lines[0].type, IniLineType::Section);
    QCOMPARE(result.snapshot.lines[1].type, IniLineType::KeyValue);
    QCOMPARE(result.snapshot.lines[2].type, IniLineType::KeyValue);
    QCOMPARE(result.snapshot.lines[4].type, IniLineType::Malformed);
}

void IniCoreTests::serializer_round_trip_preserves_structure() {
    IniParser parser;
    IniSerializer serializer;
    const QByteArray text =
        "\xEF\xBB\xBF"
        "[General]\r\n"
        "foo=bar\r\n"
        "\r\n"
        "; note\r\n";
    const auto parsed = parser.parseText(text);
    QVERIFY(parsed.ok);

    IniDocument doc;
    doc.restore(parsed.snapshot);
    doc.setDirty(false);
    const QByteArray out = serializer.serialize(doc);
    QVERIFY(out.startsWith("\xEF\xBB\xBF"));
    QVERIFY(out.contains("[General]\r\nfoo=bar\r\n\r\n; note"));
}

void IniCoreTests::semantic_diff_detects_added_removed_changed_unchanged() {
    IniParser parser;
    IniDocument baseDoc;
    IniDocument compareDoc;

    auto base = parser.parseText("[A]\nK1=1\nK2=2\nK4=4\n");
    auto compare = parser.parseText("[A]\nK1=1\nK2=22\nK3=3\n");
    QVERIFY(base.ok);
    QVERIFY(compare.ok);
    baseDoc.restore(base.snapshot);
    compareDoc.restore(compare.snapshot);

    IniDiffService diff;
    const auto items = diff.buildSemanticDiff(baseDoc, compareDoc, true);
    QVERIFY(!items.isEmpty());

    bool seenAdded = false;
    bool seenRemoved = false;
    bool seenChanged = false;
    bool seenUnchanged = false;
    for (const auto& item : items) {
        seenAdded |= item.status == SemanticDiffStatus::Added;
        seenRemoved |= item.status == SemanticDiffStatus::Removed;
        seenChanged |= item.status == SemanticDiffStatus::Changed;
        seenUnchanged |= item.status == SemanticDiffStatus::Unchanged;
    }
    QVERIFY(seenAdded);
    QVERIFY(seenRemoved);
    QVERIFY(seenChanged);
    QVERIFY(seenUnchanged);
}

void IniCoreTests::merge_policies_apply_as_expected() {
    IniParser parser;
    IniDocument target;
    auto targetParse = parser.parseText("[A]\nK1=old\n");
    QVERIFY(targetParse.ok);
    target.restore(targetParse.snapshot);

    QVector<MergePreviewItem> preview{
        MergePreviewItem{
            .selected = true,
            .section = "A",
            .key = "K1",
            .sourceValue = "new",
            .targetValue = "old",
            .status = SemanticDiffStatus::Changed,
        },
        MergePreviewItem{
            .selected = true,
            .section = "A",
            .key = "K2",
            .sourceValue = "added",
            .targetValue = "",
            .status = SemanticDiffStatus::Added,
        },
    };

    IniMergeService merge;
    const auto summaryKeep = merge.applyPreview(target, preview, MergeConflictPolicy::KeepTarget);
    QCOMPARE(target.getSettingValue("A", "K1"), QString("old"));
    QCOMPARE(target.getSettingValue("A", "K2"), QString("added"));
    QCOMPARE(summaryKeep.kept, 1);
    QCOMPARE(summaryKeep.added, 1);
}

void IniCoreTests::integration_diff_and_merge_from_sample_files() {
    IniParser parser;
    QFile baseFile(QStringLiteral(TEST_DATA_DIR) + "/base.ini");
    QFile modFile(QStringLiteral(TEST_DATA_DIR) + "/mod.ini");
    QVERIFY(baseFile.open(QIODevice::ReadOnly));
    QVERIFY(modFile.open(QIODevice::ReadOnly));

    auto base = parser.parseText(baseFile.readAll(), baseFile.fileName());
    auto mod = parser.parseText(modFile.readAll(), modFile.fileName());
    QVERIFY(base.ok);
    QVERIFY(mod.ok);

    IniDocument target;
    IniDocument source;
    target.restore(base.snapshot);
    source.restore(mod.snapshot);

    IniDiffService diff;
    IniMergeService merge;
    const auto semantic = diff.buildSemanticDiff(target, source, true);
    const auto preview = merge.buildPreview(semantic);
    const auto summary = merge.applyPreview(target, preview, MergeConflictPolicy::ReplaceTarget);

    QVERIFY(summary.added > 0 || summary.replaced > 0);
    QCOMPARE(target.getSettingValue("Display", "iSize W"), QString("2560"));
    QCOMPARE(target.getSettingValue("Display", "iSize H"), QString("1440"));
}

QTEST_MAIN(IniCoreTests)
#include "IniCoreTests.moc"

