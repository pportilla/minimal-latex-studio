// SPDX-License-Identifier: MIT

#include <poppler-qt6.h>
#include <qtermwidget.h>

#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QFontDatabase>
#include <QFormLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLayoutItem>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QMap>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProcessEnvironment>
#include <QPushButton>
#include <QRegularExpression>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QSet>
#include <QSettings>
#include <QShortcut>
#include <QSpinBox>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QSyntaxHighlighter>
#include <QTabWidget>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextFormat>
#include <QTextLayout>
#include <QTextStream>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QVariant>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <utility>

namespace {

constexpr auto kOrgName = "LiTeX";
constexpr auto kAppName = "LiTeX";
constexpr int kDefaultEditorFontSize = 12;
constexpr int kDefaultTerminalFontSize = 12;
constexpr int kMinTextFontSize = 8;
constexpr int kMaxTextFontSize = 28;

QString cssColor(const QColor& color)
{
    return color.name(QColor::HexRgb);
}

QColor readColor(const QJsonObject& object, const QString& key, const QColor& fallback)
{
    const auto value = object.value(key).toString();
    const QColor color(value);
    return color.isValid() ? color : fallback;
}

QString latexLineWithoutComment(const QString& line)
{
    for (int i = 0; i < line.size(); ++i) {
        if (line.at(i) == '%' && (i == 0 || line.at(i - 1) != '\\')) {
            return line.left(i);
        }
    }
    return line;
}

enum class UiIcon : std::uint8_t {
    SidebarLeft = 0,
    SidebarRight,
    NewFile,
    OpenFile,
    Save,
    Build,
    Clean,
    Pdf,
    Options,
    Folder,
    Refresh,
    ZoomOut,
    ZoomIn,
    FitWidth
};

constexpr auto kUiIconProperty = "minimalLatexStudioIcon";

QPixmap drawUiIconPixmap(UiIcon icon, const QColor& color, int size = 24)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPen pen(color, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    auto drawFile = [&] {
        QPainterPath path;
        path.moveTo(7, 4);
        path.lineTo(14, 4);
        path.lineTo(18, 8);
        path.lineTo(18, 20);
        path.lineTo(7, 20);
        path.closeSubpath();
        painter.drawPath(path);
        painter.drawLine(QPointF(14, 4), QPointF(14, 8));
        painter.drawLine(QPointF(14, 8), QPointF(18, 8));
    };

    auto drawFolder = [&] {
        QPainterPath path;
        path.moveTo(4, 8);
        path.lineTo(9, 8);
        path.lineTo(11, 6);
        path.lineTo(16, 6);
        path.lineTo(18, 8);
        path.lineTo(20, 8);
        path.lineTo(20, 18);
        path.lineTo(4, 18);
        path.closeSubpath();
        painter.drawPath(path);
    };

    switch (icon) {
    case UiIcon::SidebarLeft:
        painter.drawLine(QPointF(15, 6), QPointF(9, 12));
        painter.drawLine(QPointF(9, 12), QPointF(15, 18));
        break;
    case UiIcon::SidebarRight:
        painter.drawLine(QPointF(9, 6), QPointF(15, 12));
        painter.drawLine(QPointF(15, 12), QPointF(9, 18));
        break;
    case UiIcon::NewFile:
        drawFile();
        painter.drawLine(QPointF(9.5, 13), QPointF(15.5, 13));
        painter.drawLine(QPointF(12.5, 10), QPointF(12.5, 16));
        break;
    case UiIcon::OpenFile:
        drawFolder();
        painter.drawLine(QPointF(9, 13), QPointF(15, 13));
        painter.drawLine(QPointF(13, 11), QPointF(15, 13));
        painter.drawLine(QPointF(13, 15), QPointF(15, 13));
        break;
    case UiIcon::Save:
        painter.drawRoundedRect(QRectF(5, 4, 14, 16), 1.5, 1.5);
        painter.drawLine(QPointF(8, 4), QPointF(8, 9));
        painter.drawLine(QPointF(8, 9), QPointF(16, 9));
        painter.drawRoundedRect(QRectF(8, 14, 8, 5), 1.0, 1.0);
        break;
    case UiIcon::Build: {
        QPainterPath path;
        path.moveTo(8, 6);
        path.lineTo(18, 12);
        path.lineTo(8, 18);
        path.closeSubpath();
        painter.setBrush(color);
        painter.drawPath(path);
        break;
    }
    case UiIcon::Clean:
        painter.drawLine(QPointF(8, 7), QPointF(16, 7));
        painter.drawLine(QPointF(10, 5), QPointF(14, 5));
        painter.drawRoundedRect(QRectF(8, 9, 8, 10), 1.2, 1.2);
        painter.drawLine(QPointF(10.5, 11), QPointF(10.5, 17));
        painter.drawLine(QPointF(13.5, 11), QPointF(13.5, 17));
        break;
    case UiIcon::Pdf:
        drawFile();
        painter.setFont(QFont(
            QFontDatabase::systemFont(QFontDatabase::GeneralFont).family(), 5, QFont::DemiBold
        ));
        painter.drawText(QRectF(7, 12, 11, 6), Qt::AlignCenter, "PDF");
        break;
    case UiIcon::Options:
        painter.drawLine(QPointF(5, 7), QPointF(19, 7));
        painter.drawEllipse(QPointF(9, 7), 2, 2);
        painter.drawLine(QPointF(5, 12), QPointF(19, 12));
        painter.drawEllipse(QPointF(15, 12), 2, 2);
        painter.drawLine(QPointF(5, 17), QPointF(19, 17));
        painter.drawEllipse(QPointF(11, 17), 2, 2);
        break;
    case UiIcon::Folder:
        drawFolder();
        break;
    case UiIcon::Refresh: {
        painter.drawArc(QRectF(6, 6, 12, 12), 30 * 16, 285 * 16);
        QPainterPath arrow;
        arrow.moveTo(17, 7);
        arrow.lineTo(18, 12);
        arrow.lineTo(13, 10);
        painter.drawPath(arrow);
        break;
    }
    case UiIcon::ZoomOut:
        painter.drawEllipse(QRectF(5, 5, 10, 10));
        painter.drawLine(QPointF(13, 13), QPointF(19, 19));
        painter.drawLine(QPointF(8, 10), QPointF(12, 10));
        break;
    case UiIcon::ZoomIn:
        painter.drawEllipse(QRectF(5, 5, 10, 10));
        painter.drawLine(QPointF(13, 13), QPointF(19, 19));
        painter.drawLine(QPointF(8, 10), QPointF(12, 10));
        painter.drawLine(QPointF(10, 8), QPointF(10, 12));
        break;
    case UiIcon::FitWidth:
        painter.drawRoundedRect(QRectF(7, 5, 10, 14), 1.2, 1.2);
        painter.drawLine(QPointF(4, 12), QPointF(9, 12));
        painter.drawLine(QPointF(15, 12), QPointF(20, 12));
        painter.drawLine(QPointF(7, 10), QPointF(9, 12));
        painter.drawLine(QPointF(7, 14), QPointF(9, 12));
        painter.drawLine(QPointF(17, 10), QPointF(15, 12));
        painter.drawLine(QPointF(17, 14), QPointF(15, 12));
        break;
    }

    return pixmap;
}

QIcon makeUiIcon(UiIcon icon, const QColor& normal, const QColor& disabled, const QColor& active)
{
    QIcon result;
    result.addPixmap(drawUiIconPixmap(icon, normal), QIcon::Normal, QIcon::Off);
    result.addPixmap(drawUiIconPixmap(icon, active), QIcon::Active, QIcon::Off);
    result.addPixmap(drawUiIconPixmap(icon, disabled), QIcon::Disabled, QIcon::Off);
    result.addPixmap(drawUiIconPixmap(icon, active), QIcon::Selected, QIcon::On);
    return result;
}

void applyThemedToolIcons(
    QWidget* root, const QColor& normal, const QColor& disabled, const QColor& active
)
{
    const QList<QToolButton*> buttons = root->findChildren<QToolButton*>();
    for (QToolButton* button : buttons) {
        const QVariant iconValue = button->property(kUiIconProperty);
        if (iconValue.isValid()) {
            button->setIcon(
                makeUiIcon(static_cast<UiIcon>(iconValue.toInt()), normal, disabled, active)
            );
        }
    }
}

QToolButton* makeToolButton(QWidget* parent, UiIcon icon, const QString& tooltip)
{
    auto* button = new QToolButton(parent);
    button->setProperty(kUiIconProperty, static_cast<int>(icon));
    button->setToolTip(tooltip);
    button->setAutoRaise(true);
    button->setFocusPolicy(Qt::NoFocus);
    button->setIconSize(QSize(20, 20));
    return button;
}

QIcon loadAppIcon()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        QDir::currentPath() + "/resources/minimal-latex-studio.svg",
        appDir + "/../share/icons/hicolor/scalable/apps/minimal-latex-studio.svg",
        "/usr/share/icons/hicolor/scalable/apps/minimal-latex-studio.svg"
    };

    for (const QString& candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return QIcon(candidate);
        }
    }

    return QIcon::fromTheme("accessories-text-editor");
}

QStringList commonAuxiliaryExtensions()
{
    return {".aux", ".bbl",     ".bcf", ".blg",        ".dvi", ".fdb_latexmk", ".fls",
            ".idx", ".ilg",     ".ind", ".lof",        ".log", ".lot",         ".nav",
            ".out", ".run.xml", ".snm", ".synctex.gz", ".toc", ".vrb",         ".xdv"};
}

bool readUtf8TextFile(const QString& path, QString& text)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    text = QString::fromUtf8(file.readAll());
    return true;
}

QStringList normalizedDiffLines(QString text)
{
    text.replace("\r\n", "\n");
    text.replace('\r', '\n');
    return text.split('\n');
}

QString clippedDiffLine(const QString& line)
{
    constexpr int kMaxLineLength = 220;
    return line.size() <= kMaxLineLength ? line : line.left(kMaxLineLength) + " ...";
}

QString makeDiffPreview(const QString& editorText, const QString& diskText)
{
    constexpr int kMaxInputLines = 700;
    constexpr int kMaxOutputLines = 900;

    const QStringList editorLines = normalizedDiffLines(editorText);
    const QStringList diskLines = normalizedDiffLines(diskText);
    const int editorCount = std::min(static_cast<int>(editorLines.size()), kMaxInputLines);
    const int diskCount = std::min(static_cast<int>(diskLines.size()), kMaxInputLines);

    QVector<QVector<int>> lcs(editorCount + 1);
    for (QVector<int>& row : lcs) {
        row.resize(diskCount + 1);
    }

    for (int left = editorCount - 1; left >= 0; --left) {
        for (int right = diskCount - 1; right >= 0; --right) {
            if (editorLines.at(left) == diskLines.at(right)) {
                lcs[left][right] = lcs[left + 1][right + 1] + 1;
            } else {
                lcs[left][right] = std::max(lcs[left + 1][right], lcs[left][right + 1]);
            }
        }
    }

    QStringList output;
    output << "--- editor" << "+++ disk";
    bool hasChanges = false;
    bool truncated = false;

    auto appendLine = [&](const QString& prefix, const QString& line) {
        if (output.size() >= kMaxOutputLines) {
            truncated = true;
            return;
        }
        output << prefix + clippedDiffLine(line);
    };

    int left = 0;
    int right = 0;
    while (left < editorCount || right < diskCount) {
        if (left < editorCount && right < diskCount &&
            editorLines.at(left) == diskLines.at(right)) {
            appendLine("  ", editorLines.at(left));
            ++left;
            ++right;
        } else if (
            right >= diskCount ||
            (left < editorCount && lcs[left + 1][right] >= lcs[left][right + 1])
        ) {
            appendLine("- ", editorLines.at(left));
            hasChanges = true;
            ++left;
        } else {
            appendLine("+ ", diskLines.at(right));
            hasChanges = true;
            ++right;
        }

        if (truncated) {
            break;
        }
    }

    if (!hasChanges && editorLines.size() == diskLines.size()) {
        return "No textual differences.";
    }

    if (truncated || editorLines.size() > kMaxInputLines || diskLines.size() > kMaxInputLines) {
        output << "... diff truncated ...";
    }
    return output.join('\n');
}

} // namespace

struct AppTheme
{
    QString id = "smart-modern";
    QString name = "Smart Modern";

    QColor window = QColor("#f4f6f2");
    QColor panel = QColor("#fbfbf8");
    QColor panelAlt = QColor("#edf1ec");
    QColor text = QColor("#202522");
    QColor mutedText = QColor("#5f6963");
    QColor border = QColor("#d5ddd5");
    QColor accent = QColor("#1f7a6d");
    QColor accentHover = QColor("#175f55");
    QColor selection = QColor("#d6ebe5");

    QColor editorBackground = QColor("#fbfbf8");
    QColor editorText = QColor("#202522");
    QColor gutterBackground = QColor("#edf1ec");
    QColor gutterText = QColor("#7a857e");

    QColor terminalBackground = QColor("#000000");
    QColor terminalText = QColor("#eeeeee");
    QColor pdfBackground = QColor("#cfd7d1");

    QColor syntaxCommand = QColor("#1f7a6d");
    QColor syntaxKeyword = QColor("#405a73");
    QColor syntaxComment = QColor("#7d877f");
    QColor syntaxMath = QColor("#8f5b2f");
    QColor syntaxError = QColor("#b0443e");

    QString terminalScheme = "WhiteOnBlack";
};

class ThemeLoader
{
  public:
    static AppTheme load(const QString& path = {})
    {
        const QString resolvedPath = path.isEmpty() ? defaultThemePath() : path;
        QFile file(resolvedPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return {};
        }

        const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
        if (!document.isObject()) {
            return {};
        }

        const QJsonObject root = document.object();
        const QJsonObject colors = root.value("colors").toObject();
        const QJsonObject editor = root.value("editor").toObject();
        const QJsonObject syntax = root.value("syntax").toObject();
        const QJsonObject terminal = root.value("terminal").toObject();
        const QJsonObject pdf = root.value("pdf").toObject();

        AppTheme theme;
        theme.id = root.value("id").toString(theme.id);
        theme.name = root.value("name").toString(theme.name);

        theme.window = readColor(colors, "window", theme.window);
        theme.panel = readColor(colors, "panel", theme.panel);
        theme.panelAlt = readColor(colors, "panelAlt", theme.panelAlt);
        theme.text = readColor(colors, "text", theme.text);
        theme.mutedText = readColor(colors, "mutedText", theme.mutedText);
        theme.border = readColor(colors, "border", theme.border);
        theme.accent = readColor(colors, "accent", theme.accent);
        theme.accentHover = readColor(colors, "accentHover", theme.accentHover);
        theme.selection = readColor(colors, "selection", theme.selection);

        theme.editorBackground = readColor(editor, "background", theme.editorBackground);
        theme.editorText = readColor(editor, "text", theme.editorText);
        theme.gutterBackground = readColor(editor, "gutterBackground", theme.gutterBackground);
        theme.gutterText = readColor(editor, "gutterText", theme.gutterText);

        theme.terminalBackground = readColor(terminal, "background", theme.terminalBackground);
        theme.terminalText = readColor(terminal, "text", theme.terminalText);
        theme.terminalScheme = terminal.value("scheme").toString(theme.terminalScheme);

        theme.pdfBackground = readColor(pdf, "background", theme.pdfBackground);

        theme.syntaxCommand = readColor(syntax, "command", theme.syntaxCommand);
        theme.syntaxKeyword = readColor(syntax, "keyword", theme.syntaxKeyword);
        theme.syntaxComment = readColor(syntax, "comment", theme.syntaxComment);
        theme.syntaxMath = readColor(syntax, "math", theme.syntaxMath);
        theme.syntaxError = readColor(syntax, "error", theme.syntaxError);

        return theme;
    }

    static QString defaultThemePath()
    {
        const QString appDir = QCoreApplication::applicationDirPath();
        const QString dataPath =
            QStandardPaths::locate(QStandardPaths::AppDataLocation, "themes/smart-modern.json");
        const QStringList candidates = {
            dataPath, appDir + "/../share/minimal-latex-studio/themes/smart-modern.json",
            QDir::currentPath() + "/resources/themes/smart-modern.json",
            "/usr/share/minimal-latex-studio/themes/smart-modern.json"
        };

        for (const QString& candidate : candidates) {
            if (!candidate.isEmpty() && QFileInfo::exists(candidate)) {
                return candidate;
            }
        }

        return {};
    }
};

class LatexHighlighter final : public QSyntaxHighlighter
{
  public:
    explicit LatexHighlighter(QTextDocument* document) : QSyntaxHighlighter(document) {}

    void applyTheme(const AppTheme& theme)
    {
        rules_.clear();

        QTextCharFormat commandFormat;
        commandFormat.setForeground(theme.syntaxCommand);
        commandFormat.setFontWeight(QFont::DemiBold);
        rules_.push_back({QRegularExpression(R"(\\[A-Za-z@]+[*]?)"), commandFormat});

        QTextCharFormat environmentFormat;
        environmentFormat.setForeground(theme.syntaxKeyword);
        environmentFormat.setFontWeight(QFont::DemiBold);
        rules_.push_back({QRegularExpression(R"(\\(begin|end)\s*\{[^}]+\})"), environmentFormat});

        QTextCharFormat mathFormat;
        mathFormat.setForeground(theme.syntaxMath);
        rules_.push_back({QRegularExpression(R"((\$[^$]*\$|\\\[|\\\]|\\\(|\\\)))"), mathFormat});

        commentFormat_.setForeground(theme.syntaxComment);
        errorFormat_.setForeground(theme.syntaxError);
        errorFormat_.setUnderlineColor(theme.syntaxError);
        errorFormat_.setUnderlineStyle(QTextCharFormat::WaveUnderline);
        rehighlight();
    }

  protected:
    void highlightBlock(const QString& text) override
    {
        for (const HighlightRule& rule : rules_) {
            auto matches = rule.pattern.globalMatch(text);
            while (matches.hasNext()) {
                const QRegularExpressionMatch match = matches.next();
                setFormat(
                    static_cast<int>(match.capturedStart()),
                    static_cast<int>(match.capturedLength()), rule.format
                );
            }
        }

        const int commentStart = firstUnescapedPercent(text);
        if (commentStart >= 0) {
            setFormat(commentStart, static_cast<int>(text.length()) - commentStart, commentFormat_);
        }

        const int openBraces = static_cast<int>(text.count('{'));
        const int closeBraces = static_cast<int>(text.count('}'));
        if (std::abs(openBraces - closeBraces) > 2) {
            setFormat(0, static_cast<int>(text.length()), errorFormat_);
        }
    }

  private:
    struct HighlightRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    static int firstUnescapedPercent(const QString& text)
    {
        for (int i = 0; i < text.size(); ++i) {
            if (text.at(i) == '%' && (i == 0 || text.at(i - 1) != '\\')) {
                return i;
            }
        }
        return -1;
    }

    QVector<HighlightRule> rules_;
    QTextCharFormat commentFormat_;
    QTextCharFormat errorFormat_;
};

class SourceEditor;

class LineNumberArea final : public QWidget
{
  public:
    explicit LineNumberArea(SourceEditor* editor);

    QSize sizeHint() const override;

  protected:
    void mousePressEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

  private:
    SourceEditor* editor_;
};

class SourceEditor final : public QPlainTextEdit
{
  public:
    explicit SourceEditor(QWidget* parent = nullptr)
        : QPlainTextEdit(parent), lineNumberArea_(new LineNumberArea(this)),
          highlighter_(new LatexHighlighter(document()))
    {
        setFrameShape(QFrame::NoFrame);
        setTabStopDistance(QFontMetricsF(font()).horizontalAdvance(' ') * 2.0);
        setLineWrapMode(QPlainTextEdit::NoWrap);

        connect(
            this, &QPlainTextEdit::blockCountChanged, this, &SourceEditor::updateLineNumberAreaWidth
        );
        connect(this, &QPlainTextEdit::updateRequest, this, &SourceEditor::updateLineNumberArea);
        connect(
            this, &QPlainTextEdit::cursorPositionChanged, this, &SourceEditor::highlightCurrentLine
        );
        connect(this, &QPlainTextEdit::textChanged, this, [this] {
            if (!collapsedFoldStarts_.isEmpty()) {
                unfoldAll();
            }
        });

        updateLineNumberAreaWidth();
        highlightCurrentLine();
    }

    int lineNumberAreaWidth() const
    {
        int digits = 1;
        int max = qMax(1, blockCount());
        while (max >= 10) {
            max /= 10;
            ++digits;
        }

        return foldMarkerWidth() + 18 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    }

    void lineNumberAreaPaintEvent(QPaintEvent* event)
    {
        QPainter painter(lineNumberArea_);
        painter.setFont(font());
        painter.fillRect(event->rect(), gutterBackground_);

        QTextBlock block = firstVisibleBlock();
        int blockNumber = block.blockNumber();
        int top = static_cast<int>(blockBoundingGeometry(block).translated(contentOffset()).top());
        int bottom = top + static_cast<int>(blockBoundingRect(block).height());

        painter.setPen(gutterText_);
        while (block.isValid() && top <= event->rect().bottom()) {
            if (block.isVisible() && bottom >= event->rect().top()) {
                const FoldRange range = foldRangeForBlock(blockNumber);
                if (range.valid()) {
                    paintFoldMarker(painter, top, isFolded(blockNumber));
                }

                const QString number = QString::number(blockNumber + 1);
                painter.drawText(
                    foldMarkerWidth(), top, lineNumberArea_->width() - foldMarkerWidth() - 8,
                    fontMetrics().height(), Qt::AlignRight, number
                );
            }

            block = block.next();
            top = bottom;
            bottom = top + static_cast<int>(blockBoundingRect(block).height());
            ++blockNumber;
        }
    }

    void applyTheme(const AppTheme& theme)
    {
        gutterBackground_ = theme.gutterBackground;
        gutterText_ = theme.gutterText;

        QPalette palette = this->palette();
        palette.setColor(QPalette::Base, theme.editorBackground);
        palette.setColor(QPalette::Text, theme.editorText);
        palette.setColor(QPalette::Highlight, theme.selection);
        palette.setColor(QPalette::HighlightedText, theme.text);
        setPalette(palette);

        highlighter_->applyTheme(theme);
        lineNumberArea_->update();
    }

    void setEditorFontSize(int size)
    {
        QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        mono.setPointSize(std::clamp(size, kMinTextFontSize, kMaxTextFontSize));
        setFont(mono);
        lineNumberArea_->setFont(mono);
        setTabStopDistance(QFontMetricsF(font()).horizontalAdvance(' ') * 2.0);
        updateLineNumberAreaWidth();
        lineNumberArea_->update();
    }

    int editorFontSize() const
    {
        return font().pointSize();
    }

    void setForwardSearchCallback(std::function<void(int, int)> callback)
    {
        forwardSearchCallback_ = std::move(callback);
    }

    void jumpToLineColumn(int line, int column)
    {
        if (line < 1) {
            return;
        }

        QTextBlock block = document()->findBlockByNumber(line - 1);
        if (!block.isValid()) {
            return;
        }

        const int columnOffset = std::max(0, column - 1);
        QTextCursor cursor(block);
        cursor.setPosition(block.position() + std::min(columnOffset, block.length() - 1));
        setTextCursor(cursor);
        centerCursor();
        setFocus();
    }

    void clearFolds()
    {
        collapsedFoldStarts_.clear();
        setAllBlocksVisible();
        lineNumberArea_->update();
    }

    void toggleFoldAtCursor()
    {
        const int blockNumber = textCursor().blockNumber();
        if (foldRangeForBlock(blockNumber).valid()) {
            toggleFoldAtBlock(blockNumber);
            return;
        }

        QTextBlock block = textCursor().block().previous();
        while (block.isValid()) {
            if (block.isVisible() && foldRangeForBlock(block.blockNumber()).valid()) {
                toggleFoldAtBlock(block.blockNumber());
                return;
            }
            block = block.previous();
        }
    }

    void unfoldAll()
    {
        collapsedFoldStarts_.clear();
        setAllBlocksVisible();
        document()->markContentsDirty(0, document()->characterCount());
        lineNumberArea_->update();
        viewport()->update();
    }

    void lineNumberAreaMousePressEvent(QMouseEvent* event)
    {
        if (event->button() != Qt::LeftButton || event->pos().x() > foldMarkerWidth()) {
            return;
        }

        const int blockNumber = visibleBlockNumberAtY(event->pos().y());
        if (blockNumber < 0) {
            return;
        }

        if (foldRangeForBlock(blockNumber).valid()) {
            toggleFoldAtBlock(blockNumber);
            event->accept();
        }
    }

  protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton && event->modifiers().testFlag(Qt::ControlModifier)) {
            const QTextCursor cursor = cursorForPosition(event->pos());
            if (!cursor.isNull() && forwardSearchCallback_) {
                forwardSearchCallback_(cursor.blockNumber() + 1, cursor.positionInBlock() + 1);
                event->accept();
                return;
            }
        }

        QPlainTextEdit::mousePressEvent(event);
    }

    void resizeEvent(QResizeEvent* event) override
    {
        QPlainTextEdit::resizeEvent(event);
        const QRect cr = contentsRect();
        lineNumberArea_->setGeometry(
            QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height())
        );
    }

  private:
    struct FoldRange
    {
        int start = -1;
        int end = -1;

        bool valid() const
        {
            return start >= 0 && end > start;
        }
    };

    void updateLineNumberAreaWidth()
    {
        setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
    }

    int foldMarkerWidth() const
    {
        return std::max(18, fontMetrics().height() + 6);
    }

    void updateLineNumberArea(const QRect& rect, int dy)
    {
        if (dy != 0) {
            lineNumberArea_->scroll(0, dy);
        } else {
            lineNumberArea_->update(0, rect.y(), lineNumberArea_->width(), rect.height());
        }

        if (rect.contains(viewport()->rect())) {
            updateLineNumberAreaWidth();
        }
    }

    void highlightCurrentLine()
    {
        QList<QTextEdit::ExtraSelection> selections;
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(currentLine_);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        selections.append(selection);
        setExtraSelections(selections);
    }

    void paintFoldMarker(QPainter& painter, int top, bool folded)
    {
        painter.setFont(font());
        const int lineHeight = fontMetrics().height();
        const qreal markerSize = std::clamp(lineHeight * 0.64, 10.0, 18.0);
        const QRectF box(
            (foldMarkerWidth() - markerSize) / 2.0, top + (lineHeight - markerSize) / 2.0,
            markerSize, markerSize
        );

        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(QPen(gutterText_, std::max(1.3, markerSize / 8.0)));
        painter.setBrush(QColor(gutterText_.red(), gutterText_.green(), gutterText_.blue(), 28));
        painter.drawRoundedRect(box, markerSize / 5.0, markerSize / 5.0);
        const qreal inset = markerSize * 0.26;
        painter.drawLine(
            QPointF(box.left() + inset, box.center().y()),
            QPointF(box.right() - inset, box.center().y())
        );
        if (folded) {
            painter.drawLine(
                QPointF(box.center().x(), box.top() + inset),
                QPointF(box.center().x(), box.bottom() - inset)
            );
        }
        painter.restore();
    }

    int visibleBlockNumberAtY(int y) const
    {
        QTextBlock block = firstVisibleBlock();
        int top = static_cast<int>(blockBoundingGeometry(block).translated(contentOffset()).top());
        int bottom = top + static_cast<int>(blockBoundingRect(block).height());

        while (block.isValid()) {
            if (block.isVisible() && y >= top && y <= bottom) {
                return block.blockNumber();
            }
            block = block.next();
            top = bottom;
            bottom = top + static_cast<int>(blockBoundingRect(block).height());
        }

        return -1;
    }

    bool isFolded(int blockNumber) const
    {
        return collapsedFoldStarts_.contains(blockNumber);
    }

    void toggleFoldAtBlock(int blockNumber)
    {
        const FoldRange range = foldRangeForBlock(blockNumber);
        if (!range.valid()) {
            return;
        }

        if (isFolded(blockNumber)) {
            collapsedFoldStarts_.remove(blockNumber);
            setBlockRangeVisible(range.start + 1, range.end, true);
        } else {
            collapsedFoldStarts_.insert(blockNumber);
            setBlockRangeVisible(range.start + 1, range.end, false);
        }

        document()->markContentsDirty(0, document()->characterCount());
        lineNumberArea_->update();
        viewport()->update();
    }

    void setAllBlocksVisible()
    {
        for (QTextBlock block = document()->firstBlock(); block.isValid(); block = block.next()) {
            block.setVisible(true);
            block.setLineCount(std::max(1, block.layout() ? block.layout()->lineCount() : 1));
        }
    }

    void setBlockRangeVisible(int firstBlock, int lastBlock, bool visible)
    {
        QTextBlock block = document()->findBlockByNumber(firstBlock);
        while (block.isValid() && block.blockNumber() <= lastBlock) {
            block.setVisible(visible);
            block.setLineCount(
                visible ? std::max(1, block.layout() ? block.layout()->lineCount() : 1) : 0
            );
            block = block.next();
        }
    }

    FoldRange foldRangeForBlock(int blockNumber) const
    {
        const QTextBlock block = document()->findBlockByNumber(blockNumber);
        if (!block.isValid()) {
            return {};
        }

        const QString line = latexLineWithoutComment(block.text()).trimmed();
        if (line.isEmpty()) {
            return {};
        }

        bool hasLevel = false;
        const int level = sectionLevel(line, &hasLevel);
        if (hasLevel) {
            return sectionFoldRange(blockNumber, level);
        }

        const QString environment = beginEnvironmentName(line);
        if (!environment.isEmpty()) {
            return environmentFoldRange(blockNumber, environment);
        }

        return {};
    }

    FoldRange sectionFoldRange(int startBlock, int level) const
    {
        int endBlock = blockCount() - 1;
        for (int i = startBlock + 1; i < blockCount(); ++i) {
            const QTextBlock block = document()->findBlockByNumber(i);
            bool hasLevel = false;
            const int nextLevel =
                sectionLevel(latexLineWithoutComment(block.text()).trimmed(), &hasLevel);
            if (hasLevel && nextLevel <= level) {
                endBlock = i - 1;
                break;
            }
        }

        return {startBlock, endBlock};
    }

    FoldRange environmentFoldRange(int startBlock, const QString& environment) const
    {
        int depth = 0;
        for (int i = startBlock; i < blockCount(); ++i) {
            const QTextBlock block = document()->findBlockByNumber(i);
            const QString line = latexLineWithoutComment(block.text());
            if (beginEnvironmentName(line) == environment) {
                ++depth;
            }
            if (endsEnvironment(line, environment)) {
                --depth;
                if (depth == 0) {
                    return {startBlock, i};
                }
            }
        }

        return {};
    }

    static int sectionLevel(const QString& line, bool* ok)
    {
        static const QRegularExpression pattern(
            R"(^\s*\\(part|chapter|section|subsection|subsubsection|paragraph|subparagraph)\*?\s*(?:\[[^\]]*\])?\{)"
        );
        const QRegularExpressionMatch match = pattern.match(line);
        if (!match.hasMatch()) {
            *ok = false;
            return -1;
        }

        static const QMap<QString, int> levels = {{"part", 0},          {"chapter", 1},
                                                  {"section", 2},       {"subsection", 3},
                                                  {"subsubsection", 4}, {"paragraph", 5},
                                                  {"subparagraph", 6}};

        *ok = true;
        return levels.value(match.captured(1), 99);
    }

    static QString beginEnvironmentName(const QString& line)
    {
        static const QRegularExpression pattern(R"(\\begin\{([^}]+)\})");
        const QRegularExpressionMatch match = pattern.match(line);
        if (!match.hasMatch()) {
            return {};
        }

        const QString name = match.captured(1).trimmed();
        return name == "document" ? QString() : name;
    }

    static bool endsEnvironment(const QString& line, const QString& environment)
    {
        const QRegularExpression pattern(
            QStringLiteral(R"(\\end\{%1\})").arg(QRegularExpression::escape(environment))
        );
        return pattern.match(line).hasMatch();
    }

    QWidget* lineNumberArea_;
    LatexHighlighter* highlighter_;
    std::function<void(int, int)> forwardSearchCallback_;
    QSet<int> collapsedFoldStarts_;
    QColor gutterBackground_ = QColor("#edf1ec");
    QColor gutterText_ = QColor("#7a857e");
    QColor currentLine_ = QColor(0, 0, 0, 12);
};

LineNumberArea::LineNumberArea(SourceEditor* editor) : QWidget(editor), editor_(editor) {}

QSize LineNumberArea::sizeHint() const
{
    return {editor_->lineNumberAreaWidth(), 0};
}

void LineNumberArea::paintEvent(QPaintEvent* event)
{
    editor_->lineNumberAreaPaintEvent(event);
}

void LineNumberArea::mousePressEvent(QMouseEvent* event)
{
    editor_->lineNumberAreaMousePressEvent(event);
}

class PdfPageLabel final : public QLabel
{
  public:
    explicit PdfPageLabel(int pageNumber, const QSizeF& pageSizePoints, QWidget* parent = nullptr)
        : QLabel(parent), pageNumber_(pageNumber), pageSizePoints_(pageSizePoints)
    {
        setAlignment(Qt::AlignCenter);
        setMouseTracking(true);
    }

    void setRenderedImage(const QImage& image)
    {
        imageSize_ = image.size();
        setPixmap(QPixmap::fromImage(image));
    }

    void setInverseSearchCallback(std::function<void(int, double, double)> callback)
    {
        inverseSearchCallback_ = std::move(callback);
    }

    void setSyncPoint(const QPointF& point)
    {
        syncPoint_ = point;
        update();
    }

    void clearSyncPoint()
    {
        syncPoint_.reset();
        update();
    }

    QPoint pointForPagePoint(const QPointF& point) const
    {
        if (pageSizePoints_.isEmpty() || imageSize_.isEmpty()) {
            return {};
        }

        return QPoint(
            static_cast<int>((point.x() / pageSizePoints_.width()) * imageSize_.width()),
            static_cast<int>((point.y() / pageSizePoints_.height()) * imageSize_.height())
        );
    }

    int pageNumber() const
    {
        return pageNumber_;
    }

  protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton && event->modifiers().testFlag(Qt::ControlModifier) &&
            inverseSearchCallback_ && !pageSizePoints_.isEmpty() && !imageSize_.isEmpty()) {
            const QPointF pagePoint(
                (static_cast<double>(event->position().x()) / imageSize_.width()) *
                    pageSizePoints_.width(),
                (static_cast<double>(event->position().y()) / imageSize_.height()) *
                    pageSizePoints_.height()
            );
            inverseSearchCallback_(pageNumber_, pagePoint.x(), pagePoint.y());
            event->accept();
            return;
        }

        QLabel::mousePressEvent(event);
    }

    void paintEvent(QPaintEvent* event) override
    {
        QLabel::paintEvent(event);

        if (!syncPoint_) {
            return;
        }

        const QPoint point = pointForPagePoint(*syncPoint_);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(QPen(QColor(31, 122, 109), 2.4));
        painter.setBrush(QColor(31, 122, 109, 42));
        painter.drawEllipse(point, 12, 12);
        painter.setPen(QPen(QColor(255, 255, 255, 220), 1.2));
        painter.drawEllipse(point, 5, 5);
    }

  private:
    int pageNumber_;
    QSizeF pageSizePoints_;
    QSize imageSize_;
    std::optional<QPointF> syncPoint_;
    std::function<void(int, double, double)> inverseSearchCallback_;
};

class PdfPreview final : public QWidget
{
  public:
    explicit PdfPreview(QWidget* parent = nullptr)
        : QWidget(parent), scrollArea_(new QScrollArea(this)),
          pageContainer_(new QWidget(scrollArea_)), pageLayout_(new QVBoxLayout(pageContainer_)),
          statusLabel_(new QLabel(this))
    {
        resizeDebounce_.setSingleShot(true);
        connect(&resizeDebounce_, &QTimer::timeout, this, [this] { render(); });

        pageLayout_->setContentsMargins(24, 24, 24, 24);
        pageLayout_->setSpacing(18);
        pageLayout_->addStretch();

        scrollArea_->setWidget(pageContainer_);
        scrollArea_->setWidgetResizable(true);
        scrollArea_->setFrameShape(QFrame::NoFrame);

        auto* header = new QWidget(this);
        auto* headerLayout = new QHBoxLayout(header);
        headerLayout->setContentsMargins(10, 6, 10, 6);
        headerLayout->setSpacing(6);

        auto* refreshButton = makeToolButton(this, UiIcon::Refresh, "Refresh PDF");
        auto* zoomOutButton = makeToolButton(this, UiIcon::ZoomOut, "Zoom out");
        auto* zoomInButton = makeToolButton(this, UiIcon::ZoomIn, "Zoom in");
        fitButton_ = makeToolButton(this, UiIcon::FitWidth, "Fit width");
        fitButton_->setCheckable(true);
        fitButton_->setChecked(true);

        headerLayout->addWidget(new QLabel("PDF", this));
        headerLayout->addStretch();
        headerLayout->addWidget(refreshButton);
        headerLayout->addWidget(zoomOutButton);
        headerLayout->addWidget(zoomInButton);
        headerLayout->addWidget(fitButton_);

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(header);
        layout->addWidget(scrollArea_, 1);
        layout->addWidget(statusLabel_);

        connect(refreshButton, &QToolButton::clicked, this, [this] { reload(); });
        connect(zoomOutButton, &QToolButton::clicked, this, [this] {
            fitWidth_ = false;
            fitButton_->setChecked(false);
            zoom_ = std::max(0.35, zoom_ - 0.15);
            render();
        });
        connect(zoomInButton, &QToolButton::clicked, this, [this] {
            fitWidth_ = false;
            fitButton_->setChecked(false);
            zoom_ = std::min(3.0, zoom_ + 0.15);
            render();
        });
        connect(fitButton_, &QToolButton::toggled, this, [this](bool checked) {
            fitWidth_ = checked;
            render();
        });
    }

    void applyTheme(const AppTheme& theme)
    {
        theme_ = theme;
        scrollArea_->setStyleSheet(
            QString("QScrollArea { background: %1; }").arg(cssColor(theme.pdfBackground))
        );
        pageContainer_->setStyleSheet(
            QString("QWidget { background: %1; }").arg(cssColor(theme.pdfBackground))
        );
        statusLabel_->setStyleSheet(QString("padding: 4px 10px; color: %1; background: %2;")
                                        .arg(cssColor(theme.mutedText), cssColor(theme.panelAlt)));
        applyThemedToolIcons(this, theme.text, theme.mutedText, theme.accent);
    }

    void loadPdf(const QString& path)
    {
        pdfPath_ = path;
        reload();
    }

    void setInverseSearchCallback(std::function<void(int, double, double)> callback)
    {
        inverseSearchCallback_ = std::move(callback);
        for (PdfPageLabel* label : pageLabels_) {
            label->setInverseSearchCallback(inverseSearchCallback_);
        }
    }

    void showSyncPoint(int pageNumber, double x, double y)
    {
        pendingSyncPage_ = pageNumber;
        pendingSyncPoint_ = QPointF(x, y);
        applySyncPoint();
    }

    void reload()
    {
        if (pdfPath_.isEmpty() || !QFileInfo::exists(pdfPath_)) {
            document_.reset();
            clearPages();
            statusLabel_->setText("No PDF");
            return;
        }

        document_ = Poppler::Document::load(pdfPath_);
        if (!document_) {
            clearPages();
            statusLabel_->setText("PDF could not be loaded");
            return;
        }

        document_->setRenderHint(Poppler::Document::Antialiasing, true);
        document_->setRenderHint(Poppler::Document::TextAntialiasing, true);
        render();
    }

    QString currentPath() const
    {
        return pdfPath_;
    }

  protected:
    void resizeEvent(QResizeEvent* event) override
    {
        QWidget::resizeEvent(event);
        if (fitWidth_ && document_) {
            resizeDebounce_.start(140);
        }
    }

  private:
    void clearPages()
    {
        pageLabels_.clear();
        while (QLayoutItem* item = pageLayout_->takeAt(0)) {
            if (QWidget* widget = item->widget()) {
                widget->deleteLater();
            }
            delete item;
        }
        pageLayout_->addStretch();
    }

    void render()
    {
        clearPages();
        if (!document_) {
            return;
        }

        const int pageCount = document_->numPages();
        const int targetWidth = std::max(280, scrollArea_->viewport()->width() - 54);
        for (int i = 0; i < pageCount; ++i) {
            std::unique_ptr<Poppler::Page> page = document_->page(i);
            if (!page) {
                continue;
            }

            const QSizeF points = page->pageSizeF();
            double resolution = 72.0 * zoom_;
            if (fitWidth_ && points.width() > 0.0) {
                resolution = (static_cast<double>(targetWidth) / points.width()) * 72.0;
            }
            resolution = std::clamp(resolution, 48.0, 240.0);

            QImage image = page->renderToImage(resolution, resolution);
            if (image.isNull()) {
                continue;
            }

            auto* label = new PdfPageLabel(i + 1, points, pageContainer_);
            label->setRenderedImage(image);
            label->setInverseSearchCallback(inverseSearchCallback_);
            label->setStyleSheet(
                QString("QLabel { background: white; border: 1px solid %1; padding: 0; }")
                    .arg(cssColor(theme_.border))
            );
            pageLabels_.push_back(label);
            pageLayout_->insertWidget(pageLayout_->count() - 1, label, 0, Qt::AlignHCenter);
        }

        const QFileInfo info(pdfPath_);
        statusLabel_->setText(QString("%1 page%2 - %3")
                                  .arg(pageCount)
                                  .arg(pageCount == 1 ? "" : "s")
                                  .arg(info.fileName()));

        applySyncPoint();
    }

    void applySyncPoint()
    {
        if (!pendingSyncPage_ || !pendingSyncPoint_) {
            return;
        }

        PdfPageLabel* target = nullptr;
        for (PdfPageLabel* label : pageLabels_) {
            label->clearSyncPoint();
            if (label && label->pageNumber() == *pendingSyncPage_) {
                target = label;
            }
        }

        if (!target) {
            return;
        }

        target->setSyncPoint(*pendingSyncPoint_);
        const QPoint point = target->pointForPagePoint(*pendingSyncPoint_);
        const QPoint containerPoint = target->mapTo(pageContainer_, point);
        scrollArea_->horizontalScrollBar()->setValue(
            std::max(0, containerPoint.x() - scrollArea_->viewport()->width() / 2)
        );
        scrollArea_->verticalScrollBar()->setValue(
            std::max(0, containerPoint.y() - scrollArea_->viewport()->height() / 2)
        );
    }

    QScrollArea* scrollArea_;
    QWidget* pageContainer_;
    QVBoxLayout* pageLayout_;
    QLabel* statusLabel_;
    QToolButton* fitButton_ = nullptr;
    QTimer resizeDebounce_;
    std::unique_ptr<Poppler::Document> document_;
    QVector<PdfPageLabel*> pageLabels_;
    std::optional<int> pendingSyncPage_;
    std::optional<QPointF> pendingSyncPoint_;
    std::function<void(int, double, double)> inverseSearchCallback_;
    QString pdfPath_;
    double zoom_ = 1.0;
    bool fitWidth_ = true;
    AppTheme theme_;
};

struct CompilerInfo
{
    QString id;
    QString label;
    QString executable;
};

class MainWindow final : public QMainWindow
{
  public:
    explicit MainWindow(QWidget* parent = nullptr)
        : QMainWindow(parent), settings_(kOrgName, kAppName)
    {
        setWindowTitle(kAppName);
        setWindowIcon(loadAppIcon());
        resize(1380, 860);
        createUi();
        setupSourceWatcher();
        loadSettings();
        detectCompilers();
        applyTheme();
        newDocument(false);
        refreshProjectFiles();
        setupTerminal();
        setupShortcuts();
    }

    void openInitialFile(const QString& path)
    {
        if (QFileInfo::exists(path)) {
            loadFile(path);
        }
    }

  protected:
    void closeEvent(QCloseEvent* event) override
    {
        if (!maybeSave()) {
            event->ignore();
            return;
        }
        saveSettings();
        QMainWindow::closeEvent(event);
    }

  private:
    struct BuildContext
    {
        QString compilerId;
        QString executable;
        QString mode;
        QString dir;
        QString texName;
        QString baseName;
        QString pdfPath;
        int engineRuns = 0;
        bool bibliographyRun = false;
        QString currentOutput;
        QString combinedOutput;
        QString activeStep;
    };

    struct SyncTexViewResult
    {
        bool ok = false;
        int page = 0;
        double x = 0.0;
        double y = 0.0;
        QString error;
    };

    struct SyncTexEditResult
    {
        bool ok = false;
        QString input;
        int line = 0;
        int column = 0;
        QString error;
    };

    enum class DiskChangeAction : std::uint8_t { KeepEditor, ReloadFromDisk, SaveEditor };

    void createUi()
    {
        auto* central = new QWidget(this);
        auto* root = new QVBoxLayout(central);
        root->setContentsMargins(0, 0, 0, 0);
        root->setSpacing(0);

        root->addWidget(createTopBar());

        verticalSplitter_ = new QSplitter(Qt::Vertical, central);
        topSplitter_ = new QSplitter(Qt::Horizontal, verticalSplitter_);

        sidebar_ = createSidebar();
        editor_ = new SourceEditor(topSplitter_);
        pdfPreview_ = new PdfPreview(topSplitter_);
        editor_->setForwardSearchCallback([this](int line, int column) {
            forwardSyncToPdf(line, column);
        });
        pdfPreview_->setInverseSearchCallback([this](int page, double x, double y) {
            inverseSyncToEditor(page, x, y);
        });

        topSplitter_->addWidget(sidebar_);
        topSplitter_->addWidget(editor_);
        topSplitter_->addWidget(pdfPreview_);
        topSplitter_->setCollapsible(0, true);
        topSplitter_->setCollapsible(1, false);
        topSplitter_->setCollapsible(2, true);
        topSplitter_->setSizes({220, 660, 500});

        bottomTabs_ = new QTabWidget(verticalSplitter_);
        buildLog_ = new QPlainTextEdit(bottomTabs_);
        buildLog_->setReadOnly(true);
        buildLog_->setFrameShape(QFrame::NoFrame);
        bottomTabs_->addTab(buildLog_, "Log");

        terminalHost_ = new QWidget(bottomTabs_);
        auto* terminalLayout = new QVBoxLayout(terminalHost_);
        terminalLayout->setContentsMargins(0, 0, 0, 0);
        bottomTabs_->addTab(terminalHost_, "Terminal");

        verticalSplitter_->addWidget(topSplitter_);
        verticalSplitter_->addWidget(bottomTabs_);
        verticalSplitter_->setCollapsible(0, false);
        verticalSplitter_->setCollapsible(1, true);
        verticalSplitter_->setSizes({640, 220});

        root->addWidget(verticalSplitter_, 1);
        setCentralWidget(central);

        connect(editor_, &QPlainTextEdit::modificationChanged, this, [this] { updateTitle(); });
    }

    QWidget* createTopBar()
    {
        auto* bar = new QWidget(this);
        bar->setObjectName("topBar");
        auto* layout = new QHBoxLayout(bar);
        layout->setContentsMargins(10, 6, 10, 6);
        layout->setSpacing(8);

        toggleSidebarButton_ = makeToolButton(bar, UiIcon::SidebarLeft, "Toggle project panel");
        auto* newButton = makeToolButton(bar, UiIcon::NewFile, "New file");
        auto* openButton = makeToolButton(bar, UiIcon::OpenFile, "Open file");
        auto* saveButton = makeToolButton(bar, UiIcon::Save, "Save file");
        auto* buildButton = makeToolButton(bar, UiIcon::Build, "Build PDF (F1)");
        auto* cleanButton = makeToolButton(bar, UiIcon::Clean, "Clean auxiliary files");
        togglePdfButton_ = makeToolButton(bar, UiIcon::Pdf, "Toggle PDF preview");
        togglePdfButton_->setCheckable(true);
        togglePdfButton_->setChecked(true);
        auto* optionsButton = makeToolButton(bar, UiIcon::Options, "Options");

        compilerCombo_ = new QComboBox(bar);
        compilerCombo_->setToolTip("Compiler");
        compilerCombo_->setMinimumWidth(150);

        buildModeCombo_ = new QComboBox(bar);
        buildModeCombo_->setToolTip("Build mode");
        buildModeCombo_->addItem("Auto rerun", "auto");
        buildModeCombo_->addItem("Single pass", "single");
        buildModeCombo_->addItem("Force reruns", "force");
        buildModeCombo_->setMinimumWidth(140);

        statusPill_ = new QLabel("Ready", bar);
        statusPill_->setObjectName("statusPill");

        layout->addWidget(toggleSidebarButton_);
        layout->addSpacing(4);
        layout->addWidget(newButton);
        layout->addWidget(openButton);
        layout->addWidget(saveButton);
        layout->addSpacing(8);
        layout->addWidget(buildButton);
        layout->addWidget(cleanButton);
        layout->addWidget(compilerCombo_);
        layout->addWidget(buildModeCombo_);
        layout->addSpacing(8);
        layout->addWidget(togglePdfButton_);
        layout->addStretch();
        layout->addWidget(statusPill_);
        layout->addWidget(optionsButton);

        connect(toggleSidebarButton_, &QToolButton::clicked, this, [this] {
            setSidebarVisible(!sidebar_->isVisible());
        });
        connect(newButton, &QToolButton::clicked, this, [this] { newDocument(true); });
        connect(openButton, &QToolButton::clicked, this, [this] { openDocument(); });
        connect(saveButton, &QToolButton::clicked, this, [this] { saveDocument(); });
        connect(buildButton, &QToolButton::clicked, this, [this] { buildDocument(); });
        connect(cleanButton, &QToolButton::clicked, this, [this] { cleanAuxiliaryFiles(); });
        connect(togglePdfButton_, &QToolButton::toggled, this, [this](bool visible) {
            setPdfVisible(visible);
        });
        connect(optionsButton, &QToolButton::clicked, this, [this] { showOptionsDialog(); });
        connect(buildModeCombo_, &QComboBox::currentIndexChanged, this, [this] {
            settings_.setValue("buildMode", selectedBuildMode());
        });
        connect(compilerCombo_, &QComboBox::currentIndexChanged, this, [this] {
            settings_.setValue("compiler", selectedCompilerId());
        });

        return bar;
    }

    QWidget* createSidebar()
    {
        auto* sidebar = new QWidget(this);
        sidebar->setObjectName("sidebar");
        auto* layout = new QVBoxLayout(sidebar);
        layout->setContentsMargins(10, 10, 10, 10);
        layout->setSpacing(8);

        auto* header = new QWidget(sidebar);
        auto* headerLayout = new QHBoxLayout(header);
        headerLayout->setContentsMargins(0, 0, 0, 0);
        auto* title = new QLabel("Project", header);
        title->setObjectName("sidebarTitle");
        auto* openFolderButton = makeToolButton(header, UiIcon::Folder, "Open folder");
        auto* refreshButton = makeToolButton(header, UiIcon::Refresh, "Refresh files");
        headerLayout->addWidget(title);
        headerLayout->addStretch();
        headerLayout->addWidget(openFolderButton);
        headerLayout->addWidget(refreshButton);

        fileList_ = new QListWidget(sidebar);
        fileList_->setFrameShape(QFrame::NoFrame);
        fileList_->setAlternatingRowColors(false);

        layout->addWidget(header);
        layout->addWidget(fileList_, 1);

        connect(openFolderButton, &QToolButton::clicked, this, [this] { openProjectFolder(); });
        connect(refreshButton, &QToolButton::clicked, this, [this] { refreshProjectFiles(); });
        connect(fileList_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
            const QString path = item->data(Qt::UserRole).toString();
            if (!path.isEmpty()) {
                loadFile(path);
            }
        });

        return sidebar;
    }

    void setupTerminal()
    {
        terminal_ = new QTermWidget(0, terminalHost_);
        terminal_->setObjectName("terminalWidget");
        terminal_->setShellProgram(qEnvironmentVariable("SHELL", "/bin/bash"));
        terminal_->setWorkingDirectory(currentDirectory());

        QStringList env = QProcessEnvironment::systemEnvironment().toStringList();
        env.erase(
            std::remove_if(
                env.begin(), env.end(),
                [](const QString& entry) {
                    return entry.startsWith("TERM=") || entry.startsWith("COLORTERM=");
                }
            ),
            env.end()
        );
        env << "TERM=xterm-256color" << "COLORTERM=truecolor";
        terminal_->setEnvironment(env);
        terminal_->setColorScheme(
            theme_.terminalScheme.isEmpty() ? QStringLiteral("WhiteOnBlack") : theme_.terminalScheme
        );
        terminal_->setTerminalOpacity(1.0);
        applyTerminalFontSize(terminalFontSize_, false);
        terminal_->startShellProgram();

        auto* layout = qobject_cast<QVBoxLayout*>(terminalHost_->layout());
        layout->addWidget(terminal_);
        applyTerminalTheme();
    }

    void setupShortcuts()
    {
        addShortcut(QKeySequence::New, [this] { newDocument(true); });
        addShortcut(QKeySequence::Open, [this] { openDocument(); });
        addShortcut(QKeySequence::Save, [this] { saveDocument(); });
        addShortcut(QKeySequence(QStringLiteral("F1")), [this] { buildDocument(); });
        addShortcut(QKeySequence(QStringLiteral("Ctrl+B")), [this] { buildDocument(); });
        addShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+K")), [this] {
            cleanAuxiliaryFiles();
        });
        addShortcut(QKeySequence(QStringLiteral("Ctrl++")), [this] { adjustFocusedFontSize(1); });
        addShortcut(QKeySequence(QStringLiteral("Ctrl+=")), [this] { adjustFocusedFontSize(1); });
        addShortcut(QKeySequence(QStringLiteral("Ctrl+-")), [this] { adjustFocusedFontSize(-1); });
        addShortcut(QKeySequence(QStringLiteral("Ctrl+0")), [this] { resetFocusedFontSize(); });
        addShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+[")), [this] {
            editor_->toggleFoldAtCursor();
        });
        addShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+]")), [this] { editor_->unfoldAll(); });
    }

    void addShortcut(const QKeySequence& sequence, const std::function<void()>& callback)
    {
        auto* shortcut = new QShortcut(sequence, this);
        shortcut->setContext(Qt::WindowShortcut);
        connect(shortcut, &QShortcut::activated, this, [callback] { callback(); });
    }

    void setupSourceWatcher()
    {
        sourceWatcher_ = new QFileSystemWatcher(this);
        sourceReloadDebounce_.setSingleShot(true);
        sourceReloadDebounce_.setInterval(250);

        connect(sourceWatcher_, &QFileSystemWatcher::fileChanged, this, [this] {
            sourceReloadDebounce_.start();
        });
        connect(&sourceReloadDebounce_, &QTimer::timeout, this, [this] {
            handleSourceChangedOnDisk();
        });
    }

    enum class FontSurface : std::uint8_t { Editor, Terminal };

    FontSurface focusedFontSurface() const
    {
        QWidget* focus = QApplication::focusWidget();
        if (terminal_ && focus && (focus == terminal_ || terminal_->isAncestorOf(focus))) {
            return FontSurface::Terminal;
        }
        if (editor_ && focus && (focus == editor_ || editor_->isAncestorOf(focus))) {
            return FontSurface::Editor;
        }
        if (bottomTabs_ && bottomTabs_->currentWidget() == terminalHost_) {
            return FontSurface::Terminal;
        }
        return FontSurface::Editor;
    }

    void adjustFocusedFontSize(int delta)
    {
        if (focusedFontSurface() == FontSurface::Terminal) {
            applyTerminalFontSize(terminalFontSize_ + delta);
            setStatus(QString("Terminal text %1 pt").arg(terminalFontSize_));
            return;
        }

        applyEditorFontSize(editorFontSize_ + delta);
        setStatus(QString("Editor text %1 pt").arg(editorFontSize_));
    }

    void resetFocusedFontSize()
    {
        if (focusedFontSurface() == FontSurface::Terminal) {
            applyTerminalFontSize(kDefaultTerminalFontSize);
            setStatus(QString("Terminal text %1 pt").arg(terminalFontSize_));
            return;
        }

        applyEditorFontSize(kDefaultEditorFontSize);
        setStatus(QString("Editor text %1 pt").arg(editorFontSize_));
    }

    void applyEditorFontSize(int size, bool persist = true)
    {
        editorFontSize_ = std::clamp(size, kMinTextFontSize, kMaxTextFontSize);
        editor_->setEditorFontSize(editorFontSize_);
        if (persist) {
            settings_.setValue("editorFontSize", editorFontSize_);
        }
    }

    void applyTerminalFontSize(int size, bool persist = true)
    {
        terminalFontSize_ = std::clamp(size, kMinTextFontSize, kMaxTextFontSize);
        if (terminal_) {
            QFont font(QStringLiteral("monospace"));
            font.setStyleHint(QFont::Monospace);
            font.setFixedPitch(true);
            font.setKerning(false);
            font.setPointSize(terminalFontSize_);
            font.setLetterSpacing(QFont::PercentageSpacing, 100.0);
            terminal_->setTerminalFont(font);
        }
        if (persist) {
            settings_.setValue("terminalFontSize", terminalFontSize_);
        }
    }

    void applyTerminalTheme()
    {
        if (!terminal_) {
            return;
        }

        const QString scheme = theme_.terminalScheme.isEmpty() ? QStringLiteral("WhiteOnBlack")
                                                               : theme_.terminalScheme;
        terminal_->setColorScheme(scheme);
        terminal_->setTerminalOpacity(1.0);
        terminal_->setStyleSheet(
            QString(
                "QTermWidget#terminalWidget { background: %1; color: %2; }"
                "QTermWidget#terminalWidget QWidget { background: %1; color: %2; }"
            )
                .arg(cssColor(theme_.terminalBackground), cssColor(theme_.terminalText))
        );
        if (terminalHost_) {
            terminalHost_->setStyleSheet(
                QString("QWidget { background: %1; }").arg(cssColor(theme_.terminalBackground))
            );
        }
    }

    void forwardSyncToPdf(int line, int column)
    {
        const QString pdf = outputPdfPath();
        if (!canRunSyncTex(pdf)) {
            return;
        }

        const SyncTexViewResult result = runSyncTexView(line, column, pdf);
        if (!result.ok) {
            setStatus(result.error);
            return;
        }

        if (pdfPreview_->currentPath() != pdf) {
            pdfPreview_->loadPdf(pdf);
        }
        setPdfVisible(true);
        pdfPreview_->showSyncPoint(result.page, result.x, result.y);
        setStatus(QString("Source line %1 -> PDF page %2").arg(line).arg(result.page));
    }

    void inverseSyncToEditor(int page, double x, double y)
    {
        const QString pdf =
            pdfPreview_->currentPath().isEmpty() ? outputPdfPath() : pdfPreview_->currentPath();
        if (!canRunSyncTex(pdf)) {
            return;
        }

        const SyncTexEditResult result = runSyncTexEdit(page, x, y, pdf);
        if (!result.ok) {
            setStatus(result.error);
            return;
        }

        QString inputPath = result.input;
        if (inputPath.isEmpty()) {
            setStatus("SyncTeX did not return a source file");
            return;
        }

        QFileInfo inputInfo(inputPath);
        if (inputInfo.isRelative()) {
            inputInfo = QFileInfo(currentDirectory() + "/" + inputPath);
        }

        const QString targetPath = inputInfo.canonicalFilePath().isEmpty()
                                       ? inputInfo.absoluteFilePath()
                                       : inputInfo.canonicalFilePath();
        if (!QFileInfo::exists(targetPath)) {
            setStatus("SyncTeX source file was not found");
            return;
        }

        const QString currentPath =
            currentFile_.isEmpty() ? QString() : QFileInfo(currentFile_).canonicalFilePath();
        if (currentPath != QFileInfo(targetPath).canonicalFilePath()) {
            if (!maybeSave()) {
                return;
            }
            if (!loadFile(targetPath)) {
                return;
            }
        }

        editor_->jumpToLineColumn(result.line, result.column);
        setStatus(QString("PDF page %1 -> source line %2").arg(page).arg(result.line));
    }

    bool canRunSyncTex(const QString& pdf)
    {
        if (currentFile_.isEmpty()) {
            setStatus("Save and build before using SyncTeX");
            return false;
        }

        if (QStandardPaths::findExecutable("synctex").isEmpty()) {
            setStatus("SyncTeX command was not found");
            return false;
        }

        if (pdf.isEmpty() || !QFileInfo::exists(pdf)) {
            setStatus("Build the PDF before using SyncTeX");
            return false;
        }

        if (!syncTexFileExists(pdf)) {
            setStatus("Build with SyncTeX before navigating");
            return false;
        }

        return true;
    }

    bool syncTexFileExists(const QString& pdf) const
    {
        const QFileInfo info(pdf);
        const QString base = info.absolutePath() + "/" + info.completeBaseName();
        return QFileInfo::exists(base + ".synctex.gz") || QFileInfo::exists(base + ".synctex");
    }

    SyncTexViewResult runSyncTexView(int line, int column, const QString& pdf)
    {
        const QString input =
            QString("%1:%2:%3").arg(line).arg(std::max(1, column)).arg(currentFile_);
        const QString output =
            runSyncTex({"view", "-i", input, "-o", pdf}, QFileInfo(pdf).absolutePath());

        SyncTexViewResult result;
        result.page = syncTexValue(output, "Page").toInt(&result.ok);
        bool xOk = false;
        bool yOk = false;
        result.x = syncTexValue(output, "x").toDouble(&xOk);
        result.y = syncTexValue(output, "y").toDouble(&yOk);
        result.ok = result.ok && xOk && yOk && result.page > 0;
        if (!result.ok) {
            result.error = "No matching PDF position";
        }
        return result;
    }

    SyncTexEditResult runSyncTexEdit(int page, double x, double y, const QString& pdf)
    {
        const QString outputSpec =
            QString("%1:%2:%3:%4").arg(page).arg(x, 0, 'f', 3).arg(y, 0, 'f', 3).arg(pdf);
        const QString output =
            runSyncTex({"edit", "-o", outputSpec}, QFileInfo(pdf).absolutePath());

        SyncTexEditResult result;
        result.input = syncTexValue(output, "Input");
        bool lineOk = false;
        bool columnOk = false;
        result.line = syncTexValue(output, "Line").toInt(&lineOk);
        result.column = syncTexValue(output, "Column").toInt(&columnOk);
        if (!columnOk || result.column < 1) {
            result.column = 1;
        }
        result.ok = lineOk && result.line > 0 && !result.input.isEmpty();
        if (!result.ok) {
            result.error = "No matching source position";
        }
        return result;
    }

    QString runSyncTex(const QStringList& arguments, const QString& workingDirectory)
    {
        QProcess process;
        process.setProgram(QStandardPaths::findExecutable("synctex"));
        process.setArguments(arguments);
        process.setWorkingDirectory(workingDirectory);
        process.setProcessChannelMode(QProcess::MergedChannels);
        process.start();

        if (!process.waitForStarted(1500)) {
            return {};
        }

        if (!process.waitForFinished(3000)) {
            process.kill();
            process.waitForFinished();
            return {};
        }

        return QString::fromLocal8Bit(process.readAll());
    }

    QString syncTexValue(const QString& output, const QString& key) const
    {
        const QRegularExpression pattern(
            QStringLiteral("^%1:(.*)$").arg(QRegularExpression::escape(key)),
            QRegularExpression::MultilineOption
        );
        const QRegularExpressionMatch match = pattern.match(output);
        return match.hasMatch() ? match.captured(1).trimmed() : QString();
    }

    void detectCompilers()
    {
        const QString previous = settings_.value("compiler", "latexmk").toString();
        compilers_.clear();
        compilerCombo_->clear();

        const QList<QPair<QString, QString>> candidates = {
            {"latexmk", "latexmk"},
            {"pdflatex", "pdfLaTeX"},
            {"xelatex", "XeLaTeX"},
            {"lualatex", "LuaLaTeX"}
        };

        for (const auto& [id, label] : candidates) {
            const QString executable = QStandardPaths::findExecutable(id);
            if (!executable.isEmpty()) {
                compilers_.push_back({id, label, executable});
                compilerCombo_->addItem(label, id);
            }
        }

        if (compilers_.empty()) {
            compilerCombo_->addItem("No compiler found", "");
            compilerCombo_->setEnabled(false);
            setStatus("No compiler found");
            return;
        }

        compilerCombo_->setEnabled(true);
        const int index = compilerCombo_->findData(previous);
        compilerCombo_->setCurrentIndex(index >= 0 ? index : 0);
    }

    void loadSettings()
    {
        themePath_ = settings_.value("themePath").toString();
        theme_ = ThemeLoader::load(themePath_);

        editorFontSize_ = std::clamp(
            settings_.value("editorFontSize", kDefaultEditorFontSize).toInt(), kMinTextFontSize,
            kMaxTextFontSize
        );
        terminalFontSize_ = std::clamp(
            settings_.value("terminalFontSize", kDefaultTerminalFontSize).toInt(), kMinTextFontSize,
            kMaxTextFontSize
        );
        applyEditorFontSize(editorFontSize_, false);
        editor_->setLineWrapMode(
            settings_.value("lineWrap", false).toBool() ? QPlainTextEdit::WidgetWidth
                                                        : QPlainTextEdit::NoWrap
        );

        const QString mode = settings_.value("buildMode", "auto").toString();
        const int modeIndex = buildModeCombo_->findData(mode);
        if (modeIndex >= 0) {
            buildModeCombo_->setCurrentIndex(modeIndex);
        }

        setSidebarVisible(settings_.value("showSidebar", true).toBool());
        setPdfVisible(settings_.value("showPdf", true).toBool());
        autoReloadPdf_ = settings_.value("autoReloadPdf", true).toBool();
        autoReloadSource_ = settings_.value("autoReloadSource", true).toBool();
    }

    void saveSettings()
    {
        settings_.setValue("themePath", themePath_);
        settings_.setValue("buildMode", selectedBuildMode());
        settings_.setValue("compiler", selectedCompilerId());
        settings_.setValue("editorFontSize", editorFontSize_);
        settings_.setValue("terminalFontSize", terminalFontSize_);
        settings_.setValue("showSidebar", sidebar_->isVisible());
        settings_.setValue("showPdf", pdfPreview_->isVisible());
        settings_.setValue("autoReloadPdf", autoReloadPdf_);
        settings_.setValue("autoReloadSource", autoReloadSource_);
        settings_.setValue("lineWrap", editor_->lineWrapMode() == QPlainTextEdit::WidgetWidth);
        settings_.setValue("geometry", saveGeometry());
        settings_.setValue("splitters/top", topSplitter_->saveState());
        settings_.setValue("splitters/vertical", verticalSplitter_->saveState());
    }

    void applyTheme()
    {
        applyApplicationPalette();

        qApp->setStyleSheet(QString(R"(
            QWidget {
                background: %1;
                color: %2;
            }
            QWidget#topBar {
                background: %3;
                border-bottom: 1px solid %4;
            }
            QWidget#sidebar {
                background: %5;
                border-right: 1px solid %4;
            }
            QLabel#sidebarTitle {
                color: %2;
                font-weight: 700;
            }
            QLabel#statusPill {
                background: %5;
                color: %6;
                border: 1px solid %4;
                border-radius: 6px;
                padding: 4px 10px;
            }
            QToolButton {
                background: transparent;
                border: 1px solid transparent;
                border-radius: 6px;
                padding: 6px;
                min-width: 28px;
                min-height: 28px;
                color: %2;
            }
            QToolButton:hover {
                background: %5;
                border-color: %4;
            }
            QToolButton:pressed {
                background: %7;
                border-color: %8;
            }
            QToolButton:checked {
                background: %7;
                border-color: %8;
            }
            QPushButton {
                background: %5;
                color: %2;
                border: 1px solid %4;
                border-radius: 6px;
                padding: 6px 12px;
                min-height: 28px;
            }
            QPushButton:hover {
                background: %7;
                border-color: %8;
            }
            QPushButton:default {
                background: %8;
                color: %3;
                border-color: %8;
            }
            QComboBox, QSpinBox, QLineEdit {
                background: %3;
                color: %2;
                border: 1px solid %4;
                border-radius: 6px;
                padding: 5px 8px;
                min-height: 24px;
            }
            QComboBox::drop-down {
                border: 0;
                width: 22px;
            }
            QComboBox QAbstractItemView {
                background: %3;
                color: %2;
                border: 1px solid %4;
                selection-background-color: %7;
                selection-color: %2;
            }
            QCheckBox {
                color: %2;
                spacing: 8px;
            }
            QCheckBox::indicator {
                width: 15px;
                height: 15px;
                border: 1px solid %4;
                border-radius: 4px;
                background: %3;
            }
            QCheckBox::indicator:checked {
                background: %8;
                border-color: %8;
            }
            QListWidget, QPlainTextEdit {
                background: %3;
                color: %2;
                selection-background-color: %7;
                selection-color: %2;
            }
            QListWidget::item {
                border-radius: 5px;
                padding: 5px 6px;
            }
            QListWidget::item:selected {
                background: %7;
                color: %2;
            }
            QTabWidget::pane {
                border-top: 1px solid %4;
                background: %3;
            }
            QTabBar::tab {
                background: %5;
                color: %6;
                border: 1px solid %4;
                border-bottom: none;
                padding: 7px 12px;
                margin-right: 2px;
            }
            QTabBar::tab:selected {
                background: %3;
                color: %2;
            }
            QSplitter::handle {
                background: %4;
            }
            QScrollBar:vertical, QScrollBar:horizontal {
                background: %5;
                border: 0;
                width: 12px;
                height: 12px;
            }
            QScrollBar::handle {
                background: %9;
                border-radius: 5px;
                min-height: 24px;
            }
            QScrollBar::add-line, QScrollBar::sub-line {
                height: 0;
                width: 0;
            }
            QToolTip {
                background: %3;
                color: %2;
                border: 1px solid %4;
                border-radius: 5px;
                padding: 5px 7px;
            }
        )")
                                .arg(
                                    cssColor(theme_.window), cssColor(theme_.text),
                                    cssColor(theme_.panel), cssColor(theme_.border)
                                )
                                .arg(
                                    cssColor(theme_.panelAlt), cssColor(theme_.mutedText),
                                    cssColor(theme_.selection), cssColor(theme_.accent)
                                )
                                .arg(cssColor(theme_.mutedText)));

        editor_->applyTheme(theme_);
        pdfPreview_->applyTheme(theme_);
        buildLog_->setStyleSheet(
            QString("QPlainTextEdit { background: %1; color: %2; font-family: monospace; }")
                .arg(cssColor(theme_.terminalBackground), cssColor(theme_.terminalText))
        );

        if (terminal_) {
            applyTerminalFontSize(terminalFontSize_, false);
            applyTerminalTheme();
        }

        applyThemedToolIcons(this, theme_.text, theme_.mutedText, theme_.accent);
    }

    void applyApplicationPalette()
    {
        QPalette palette;
        palette.setColor(QPalette::Window, theme_.window);
        palette.setColor(QPalette::WindowText, theme_.text);
        palette.setColor(QPalette::Base, theme_.panel);
        palette.setColor(QPalette::AlternateBase, theme_.panelAlt);
        palette.setColor(QPalette::ToolTipBase, theme_.panel);
        palette.setColor(QPalette::ToolTipText, theme_.text);
        palette.setColor(QPalette::Text, theme_.text);
        palette.setColor(QPalette::Button, theme_.panelAlt);
        palette.setColor(QPalette::ButtonText, theme_.text);
        palette.setColor(QPalette::BrightText, theme_.syntaxError);
        palette.setColor(QPalette::Highlight, theme_.selection);
        palette.setColor(QPalette::HighlightedText, theme_.text);
        palette.setColor(QPalette::Link, theme_.accent);
        palette.setColor(QPalette::PlaceholderText, theme_.mutedText);

        palette.setColor(QPalette::Disabled, QPalette::WindowText, theme_.mutedText);
        palette.setColor(QPalette::Disabled, QPalette::Text, theme_.mutedText);
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, theme_.mutedText);

        qApp->setPalette(palette);
        setPalette(palette);
    }

    void newDocument(bool confirm)
    {
        if (confirm && !maybeSave()) {
            return;
        }

        currentFile_.clear();
        lastDiskText_.clear();
        updateWatchedSourceFile();
        editor_->setPlainText(QStringLiteral(
            "\\documentclass{article}\n"
            "\\usepackage[a4paper, margin=1in]{geometry}\n"
            "\\usepackage{amsmath}\n"
            "\\usepackage{hyperref}\n\n"
            "\\title{Untitled}\n"
            "\\author{}\n"
            "\\date{\\today}\n\n"
            "\\begin{document}\n"
            "\\maketitle\n\n"
            "\\section{Draft}\n\n"
            "Write here.\n\n"
            "\\end{document}\n"
        ));
        editor_->clearFolds();
        editor_->document()->setModified(false);
        updateTitle();
        setStatus("New document");
    }

    bool maybeSave()
    {
        if (!editor_->document()->isModified()) {
            return true;
        }

        const QMessageBox::StandardButton choice = QMessageBox::warning(
            this, kAppName, "The current document has unsaved changes.",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
        );

        if (choice == QMessageBox::Save) {
            return saveDocument();
        }
        return choice == QMessageBox::Discard;
    }

    void openDocument()
    {
        if (!maybeSave()) {
            return;
        }

        const QString file = QFileDialog::getOpenFileName(
            this, "Open TeX file",
            projectDirectory_.isEmpty() ? QDir::homePath() : projectDirectory_,
            "TeX files (*.tex);;All files (*)"
        );
        if (!file.isEmpty()) {
            loadFile(file);
        }
    }

    bool loadFile(const QString& path)
    {
        QString text;
        if (!readUtf8TextFile(path, text)) {
            QMessageBox::critical(this, kAppName, "Could not open the selected file.");
            return false;
        }

        const QFileInfo info(path);
        currentFile_ = info.absoluteFilePath();
        lastDiskText_ = text;
        editor_->setPlainText(text);
        editor_->clearFolds();
        editor_->document()->setModified(false);
        projectDirectory_ = info.absolutePath();
        updateWatchedSourceFile();
        refreshProjectFiles();
        updateTitle();
        setStatus("Opened " + info.fileName());

        const QString pdf = outputPdfPath();
        if (QFileInfo::exists(pdf)) {
            pdfPreview_->loadPdf(pdf);
        }
        if (terminal_) {
            terminal_->changeDir(projectDirectory_);
        }
        return true;
    }

    bool saveDocument()
    {
        if (currentFile_.isEmpty()) {
            return saveDocumentAs();
        }
        return saveToPath(currentFile_);
    }

    bool saveDocumentAs()
    {
        const QString file = QFileDialog::getSaveFileName(
            this, "Save TeX file",
            projectDirectory_.isEmpty() ? QDir::homePath() + "/main.tex"
                                        : projectDirectory_ + "/main.tex",
            "TeX files (*.tex);;All files (*)"
        );
        if (file.isEmpty()) {
            return false;
        }
        return saveToPath(file);
    }

    bool saveToPath(const QString& path)
    {
        const QString text = editor_->toPlainText();
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            QMessageBox::critical(this, kAppName, "Could not save the current file.");
            return false;
        }

        file.write(text.toUtf8());
        const QFileInfo info(path);
        currentFile_ = info.absoluteFilePath();
        lastDiskText_ = text;
        projectDirectory_ = info.absolutePath();
        editor_->document()->setModified(false);
        updateWatchedSourceFile();
        updateTitle();
        refreshProjectFiles();
        if (terminal_) {
            terminal_->changeDir(projectDirectory_);
        }
        setStatus("Saved");
        return true;
    }

    void updateWatchedSourceFile()
    {
        if (!sourceWatcher_) {
            return;
        }

        const QStringList watchedFiles = sourceWatcher_->files();
        if (!watchedFiles.isEmpty()) {
            sourceWatcher_->removePaths(watchedFiles);
        }
        watchedSourceFile_.clear();

        if (currentFile_.isEmpty()) {
            return;
        }

        const QFileInfo info(currentFile_);
        if (!info.exists()) {
            return;
        }

        watchedSourceFile_ = info.absoluteFilePath();
        sourceWatcher_->addPath(watchedSourceFile_);
    }

    void handleSourceChangedOnDisk()
    {
        if (currentFile_.isEmpty()) {
            return;
        }

        const QFileInfo info(currentFile_);
        const QString path = info.absoluteFilePath();
        QString diskText;
        if (!readUtf8TextFile(path, diskText)) {
            setStatus("Current file changed on disk");
            updateWatchedSourceFile();
            return;
        }

        if (diskText == lastDiskText_) {
            updateWatchedSourceFile();
            return;
        }

        const QString editorText = editor_->toPlainText();
        if (diskText == editorText) {
            lastDiskText_ = diskText;
            editor_->document()->setModified(false);
            updateTitle();
            updateWatchedSourceFile();
            return;
        }

        if (autoReloadSource_ && !editor_->document()->isModified() && !diskChangeDialogOpen_) {
            reloadEditorFromDisk(diskText, "Reloaded from disk");
            return;
        }

        if (diskChangeDialogOpen_) {
            updateWatchedSourceFile();
            return;
        }

        const DiskChangeAction action = askDiskChangeAction(diskText);
        if (action == DiskChangeAction::ReloadFromDisk) {
            reloadEditorFromDisk(diskText, "Reloaded from disk");
            return;
        }

        if (action == DiskChangeAction::SaveEditor) {
            saveToPath(currentFile_);
            return;
        }

        lastDiskText_ = diskText;
        editor_->document()->setModified(true);
        updateTitle();
        updateWatchedSourceFile();
        setStatus("Keeping editor copy");
    }

    void reloadEditorFromDisk(const QString& diskText, const QString& status)
    {
        const int cursorPosition = editor_->textCursor().position();
        const int verticalScroll = editor_->verticalScrollBar()->value();
        const int horizontalScroll = editor_->horizontalScrollBar()->value();

        editor_->setPlainText(diskText);
        editor_->clearFolds();
        QTextCursor cursor = editor_->textCursor();
        cursor.setPosition(std::min(cursorPosition, static_cast<int>(diskText.size())));
        editor_->setTextCursor(cursor);
        editor_->verticalScrollBar()->setValue(
            std::min(verticalScroll, editor_->verticalScrollBar()->maximum())
        );
        editor_->horizontalScrollBar()->setValue(
            std::min(horizontalScroll, editor_->horizontalScrollBar()->maximum())
        );

        lastDiskText_ = diskText;
        editor_->document()->setModified(false);
        refreshProjectFiles();
        updateTitle();
        updateWatchedSourceFile();
        setStatus(status);
    }

    DiskChangeAction askDiskChangeAction(const QString& diskText)
    {
        diskChangeDialogOpen_ = true;

        QDialog dialog(this);
        dialog.setWindowTitle("File changed on disk");
        auto* layout = new QVBoxLayout(&dialog);

        auto* message = new QLabel(
            "The open file was changed outside the editor. Review the diff and choose which copy "
            "to keep.",
            &dialog
        );
        message->setWordWrap(true);

        auto* diffPreview = new QPlainTextEdit(&dialog);
        diffPreview->setReadOnly(true);
        diffPreview->setLineWrapMode(QPlainTextEdit::NoWrap);
        diffPreview->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        diffPreview->setPlainText(makeDiffPreview(editor_->toPlainText(), diskText));
        diffPreview->setMinimumSize(760, 420);

        auto* buttons = new QDialogButtonBox(&dialog);
        QPushButton* reloadButton =
            buttons->addButton("Reload from Disk", QDialogButtonBox::AcceptRole);
        QPushButton* keepButton = buttons->addButton("Keep Editor", QDialogButtonBox::RejectRole);
        QPushButton* saveButton =
            buttons->addButton("Save Editor", QDialogButtonBox::DestructiveRole);

        DiskChangeAction action = DiskChangeAction::KeepEditor;
        connect(reloadButton, &QPushButton::clicked, &dialog, [&] {
            action = DiskChangeAction::ReloadFromDisk;
            dialog.accept();
        });
        connect(keepButton, &QPushButton::clicked, &dialog, [&] {
            action = DiskChangeAction::KeepEditor;
            dialog.accept();
        });
        connect(saveButton, &QPushButton::clicked, &dialog, [&] {
            action = DiskChangeAction::SaveEditor;
            dialog.accept();
        });

        layout->addWidget(message);
        layout->addWidget(diffPreview, 1);
        layout->addWidget(buttons);
        dialog.exec();

        diskChangeDialogOpen_ = false;
        return action;
    }

    void openProjectFolder()
    {
        const QString dir = QFileDialog::getExistingDirectory(
            this, "Open folder", projectDirectory_.isEmpty() ? QDir::homePath() : projectDirectory_
        );
        if (dir.isEmpty()) {
            return;
        }
        projectDirectory_ = dir;
        refreshProjectFiles();
        if (terminal_) {
            terminal_->changeDir(projectDirectory_);
        }
    }

    void refreshProjectFiles()
    {
        const QString dirPath = currentDirectory();
        QDir dir(dirPath);
        fileList_->clear();

        const QFileInfoList files =
            dir.entryInfoList({"*.tex", "*.bib", "*.sty", "*.cls"}, QDir::Files, QDir::Name);

        for (const QFileInfo& info : files) {
            auto* item = new QListWidgetItem(info.fileName(), fileList_);
            item->setData(Qt::UserRole, info.absoluteFilePath());
            if (info.absoluteFilePath() == currentFile_) {
                item->setSelected(true);
            }
        }
    }

    void buildDocument()
    {
        if (compilerCombo_->currentData().toString().isEmpty()) {
            QMessageBox::warning(this, kAppName, "No LaTeX compiler was found on PATH.");
            return;
        }

        if (!saveDocument()) {
            return;
        }

        if (process_) {
            QMessageBox::information(this, kAppName, "A build is already running.");
            return;
        }

        const CompilerInfo compiler = selectedCompiler();
        BuildContext context;
        context.compilerId = compiler.id;
        context.executable = compiler.executable;
        context.mode = selectedBuildMode();
        context.dir = QFileInfo(currentFile_).absolutePath();
        context.texName = QFileInfo(currentFile_).fileName();
        context.baseName = QFileInfo(currentFile_).completeBaseName();
        context.pdfPath = outputPdfPath();
        buildContext_ = context;

        buildLog_->clear();
        bottomTabs_->setCurrentWidget(buildLog_);
        appendLog(QString("[%1] Build started with %2 (%3)\n")
                      .arg(
                          QDateTime::currentDateTime().toString("HH:mm:ss"), compiler.label,
                          context.mode
                      ));
        setStatus("Building");

        if (compiler.id == "latexmk") {
            startLatexmkBuild();
            return;
        }

        startEnginePass();
    }

    void startLatexmkBuild()
    {
        if (!buildContext_) {
            return;
        }

        BuildContext& context = buildContext_.value();
        QStringList args = {"-pdf", "-interaction=nonstopmode", "-synctex=1", "-file-line-error"};

        if (context.mode == "force") {
            args << "-g";
        }
        args << context.texName;

        startProcess(context.executable, args, "latexmk");
    }

    void startEnginePass()
    {
        if (!buildContext_) {
            return;
        }

        BuildContext& context = buildContext_.value();
        ++context.engineRuns;
        const QStringList args = {
            "-interaction=nonstopmode", "-synctex=1", "-file-line-error", context.texName
        };

        startProcess(context.executable, args, QString("pass %1").arg(context.engineRuns));
    }

    void startBibliographyTool(const QString& tool)
    {
        if (!buildContext_) {
            return;
        }

        BuildContext& context = buildContext_.value();
        const QString executable = QStandardPaths::findExecutable(tool);
        if (executable.isEmpty()) {
            appendLog(QString("\n%1 was requested but is not installed; continuing without it.\n")
                          .arg(tool));
            context.bibliographyRun = true;
            startEnginePass();
            return;
        }

        context.bibliographyRun = true;
        startProcess(executable, {context.baseName}, tool);
    }

    void startProcess(const QString& program, const QStringList& args, const QString& label)
    {
        if (!buildContext_) {
            return;
        }

        BuildContext& context = buildContext_.value();
        process_ = std::make_unique<QProcess>(this);
        process_->setWorkingDirectory(context.dir);
        process_->setProgram(program);
        process_->setArguments(args);
        process_->setProcessChannelMode(QProcess::MergedChannels);
        context.currentOutput.clear();
        context.activeStep = label;

        connect(process_.get(), &QProcess::readyReadStandardOutput, this, [this] {
            const QString text = QString::fromLocal8Bit(process_->readAllStandardOutput());
            if (buildContext_) {
                buildContext_->currentOutput += text;
                buildContext_->combinedOutput += text;
            }
            appendLog(text);
        });

        connect(
            process_.get(), &QProcess::finished, this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
                finishProcess(exitCode, exitStatus);
            }
        );

        appendLog(QString("\n$ %1 %2\n").arg(program, args.join(' ')));
        process_->start();
    }

    void finishProcess(int exitCode, QProcess::ExitStatus exitStatus)
    {
        if (!buildContext_) {
            process_.reset();
            return;
        }

        BuildContext& context = buildContext_.value();
        const QString activeStep = context.activeStep;
        const QString lastOutput = context.currentOutput;
        process_.reset();

        if (exitStatus != QProcess::NormalExit || exitCode != 0) {
            appendLog(
                QString("\nBuild stopped during %1 (exit code %2).\n").arg(activeStep).arg(exitCode)
            );
            setStatus("Build failed");
            buildContext_.reset();
            return;
        }

        appendLog(QString("\n%1 finished.\n").arg(activeStep));

        if (context.compilerId == "latexmk") {
            finishBuildSuccessfully();
            return;
        }

        if (activeStep == "biber" || activeStep == "bibtex") {
            startEnginePass();
            return;
        }

        const QString mode = context.mode;
        if (mode == "single") {
            finishBuildSuccessfully();
            return;
        }

        if (mode == "force") {
            if (context.engineRuns < 4) {
                startEnginePass();
                return;
            }
            finishBuildSuccessfully();
            return;
        }

        if (!context.bibliographyRun && needsBibliography(lastOutput)) {
            startBibliographyTool(preferredBibliographyTool(lastOutput));
            return;
        }

        if (needsRerun(lastOutput) && context.engineRuns < 5) {
            startEnginePass();
            return;
        }

        finishBuildSuccessfully();
    }

    void finishBuildSuccessfully()
    {
        if (!buildContext_) {
            return;
        }

        const QString pdfPath = buildContext_.value().pdfPath;

        appendLog("\nBuild complete.\n");
        setStatus("Build complete");

        if (autoReloadPdf_ && QFileInfo::exists(pdfPath)) {
            pdfPreview_->loadPdf(pdfPath);
            setPdfVisible(true);
        }

        buildContext_.reset();
    }

    bool needsBibliography(const QString& output) const
    {
        static const QList<QRegularExpression> patterns = {
            QRegularExpression(
                "Please \\(re\\)run Biber", QRegularExpression::CaseInsensitiveOption
            ),
            QRegularExpression("Please run Biber", QRegularExpression::CaseInsensitiveOption),
            QRegularExpression("Please run BibTeX", QRegularExpression::CaseInsensitiveOption),
            QRegularExpression("Citation .* undefined", QRegularExpression::CaseInsensitiveOption),
            QRegularExpression(
                "There were undefined citations", QRegularExpression::CaseInsensitiveOption
            ),
            QRegularExpression("No file .*\\.bbl", QRegularExpression::CaseInsensitiveOption)
        };

        return std::any_of(
            patterns.begin(), patterns.end(), [&output](const QRegularExpression& pattern) {
                return pattern.match(output).hasMatch();
            }
        );
    }

    bool needsRerun(const QString& output) const
    {
        static const QList<QRegularExpression> patterns = {
            QRegularExpression(
                "Rerun to get cross-references right", QRegularExpression::CaseInsensitiveOption
            ),
            QRegularExpression(
                "Label\\(s\\) may have changed", QRegularExpression::CaseInsensitiveOption
            ),
            QRegularExpression("Please rerun LaTeX", QRegularExpression::CaseInsensitiveOption),
            QRegularExpression("rerunfilecheck Warning", QRegularExpression::CaseInsensitiveOption),
            QRegularExpression(
                "Table widths have changed", QRegularExpression::CaseInsensitiveOption
            )
        };

        return std::any_of(
            patterns.begin(), patterns.end(), [&output](const QRegularExpression& pattern) {
                return pattern.match(output).hasMatch();
            }
        );
    }

    QString preferredBibliographyTool(const QString& output) const
    {
        if (output.contains("Biber", Qt::CaseInsensitive)) {
            return "biber";
        }

        if (!currentFile_.isEmpty()) {
            const QFileInfo info(currentFile_);
            if (QFileInfo::exists(info.absolutePath() + "/" + info.completeBaseName() + ".bcf")) {
                return "biber";
            }

            const QString source = editor_->toPlainText();
            if (source.contains("\\addbibresource") || source.contains("biblatex")) {
                return "biber";
            }
        }

        return "bibtex";
    }

    void cleanAuxiliaryFiles()
    {
        if (currentFile_.isEmpty()) {
            QMessageBox::information(
                this, kAppName, "Save the document before cleaning auxiliary files."
            );
            return;
        }

        const QFileInfo info(currentFile_);
        int removed = 0;
        for (const QString& extension : commonAuxiliaryExtensions()) {
            QFile file(info.absolutePath() + "/" + info.completeBaseName() + extension);
            if (file.exists() && file.remove()) {
                ++removed;
            }
        }

        appendLog(
            QString("\nRemoved %1 auxiliary file%2.\n").arg(removed).arg(removed == 1 ? "" : "s")
        );
        setStatus("Aux files cleaned");
        refreshProjectFiles();
    }

    void showOptionsDialog()
    {
        QDialog dialog(this);
        dialog.setWindowTitle("Options");

        auto* layout = new QVBoxLayout(&dialog);
        auto* form = new QFormLayout();

        auto* fontSize = new QSpinBox(&dialog);
        fontSize->setRange(kMinTextFontSize, kMaxTextFontSize);
        fontSize->setValue(editorFontSize_);

        auto* terminalFontSize = new QSpinBox(&dialog);
        terminalFontSize->setRange(kMinTextFontSize, kMaxTextFontSize);
        terminalFontSize->setValue(terminalFontSize_);

        auto* lineWrap = new QCheckBox(&dialog);
        lineWrap->setChecked(editor_->lineWrapMode() == QPlainTextEdit::WidgetWidth);

        auto* showSidebar = new QCheckBox(&dialog);
        showSidebar->setChecked(sidebar_->isVisible());

        auto* showPdf = new QCheckBox(&dialog);
        showPdf->setChecked(pdfPreview_->isVisible());

        auto* autoReloadPdf = new QCheckBox(&dialog);
        autoReloadPdf->setChecked(autoReloadPdf_);

        auto* autoReloadSource = new QCheckBox(&dialog);
        autoReloadSource->setChecked(autoReloadSource_);

        auto* themePath = new QLineEdit(themePath_, &dialog);
        auto* themeBrowse = new QPushButton("Browse", &dialog);
        auto* themeRow = new QWidget(&dialog);
        auto* themeRowLayout = new QHBoxLayout(themeRow);
        themeRowLayout->setContentsMargins(0, 0, 0, 0);
        themeRowLayout->addWidget(themePath, 1);
        themeRowLayout->addWidget(themeBrowse);

        form->addRow("Editor size", fontSize);
        form->addRow("Terminal size", terminalFontSize);
        form->addRow("Line wrap", lineWrap);
        form->addRow("Project panel", showSidebar);
        form->addRow("PDF preview", showPdf);
        form->addRow("Auto reload source", autoReloadSource);
        form->addRow("Auto reload PDF", autoReloadPdf);
        form->addRow("Theme JSON", themeRow);

        auto* buttons =
            new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
        layout->addLayout(form);
        layout->addWidget(buttons);

        connect(themeBrowse, &QPushButton::clicked, &dialog, [this, themePath] {
            const QString selected = QFileDialog::getOpenFileName(
                this, "Choose theme JSON",
                themePath->text().isEmpty() ? QDir::homePath()
                                            : QFileInfo(themePath->text()).absolutePath(),
                "JSON files (*.json);;All files (*)"
            );
            if (!selected.isEmpty()) {
                themePath->setText(selected);
            }
        });
        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

        if (dialog.exec() != QDialog::Accepted) {
            return;
        }

        settings_.setValue("editorFontSize", fontSize->value());
        settings_.setValue("terminalFontSize", terminalFontSize->value());
        settings_.setValue("lineWrap", lineWrap->isChecked());
        autoReloadPdf_ = autoReloadPdf->isChecked();
        autoReloadSource_ = autoReloadSource->isChecked();
        themePath_ = themePath->text().trimmed();

        applyEditorFontSize(fontSize->value(), false);
        applyTerminalFontSize(terminalFontSize->value(), false);
        editor_->setLineWrapMode(
            lineWrap->isChecked() ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap
        );
        setSidebarVisible(showSidebar->isChecked());
        setPdfVisible(showPdf->isChecked());

        theme_ = ThemeLoader::load(themePath_);
        applyTheme();
        saveSettings();
    }

    void setSidebarVisible(bool visible)
    {
        sidebar_->setVisible(visible);
        toggleSidebarButton_->setProperty(
            kUiIconProperty, static_cast<int>(visible ? UiIcon::SidebarLeft : UiIcon::SidebarRight)
        );
        toggleSidebarButton_->setIcon(makeUiIcon(
            visible ? UiIcon::SidebarLeft : UiIcon::SidebarRight, theme_.text, theme_.mutedText,
            theme_.accent
        ));
    }

    void setPdfVisible(bool visible)
    {
        pdfPreview_->setVisible(visible);
        togglePdfButton_->setChecked(visible);
    }

    CompilerInfo selectedCompiler() const
    {
        const QString id = selectedCompilerId();
        const auto found =
            std::find_if(compilers_.begin(), compilers_.end(), [&id](const CompilerInfo& compiler) {
                return compiler.id == id;
            });
        return found == compilers_.end() ? CompilerInfo{} : *found;
    }

    QString selectedCompilerId() const
    {
        return compilerCombo_->currentData().toString();
    }

    QString selectedBuildMode() const
    {
        return buildModeCombo_->currentData().toString();
    }

    QString currentDirectory() const
    {
        if (!projectDirectory_.isEmpty()) {
            return projectDirectory_;
        }
        if (!currentFile_.isEmpty()) {
            return QFileInfo(currentFile_).absolutePath();
        }
        return QDir::homePath();
    }

    QString outputPdfPath() const
    {
        if (currentFile_.isEmpty()) {
            return {};
        }
        const QFileInfo info(currentFile_);
        return info.absolutePath() + "/" + info.completeBaseName() + ".pdf";
    }

    void appendLog(const QString& text)
    {
        buildLog_->moveCursor(QTextCursor::End);
        buildLog_->insertPlainText(text);
        buildLog_->moveCursor(QTextCursor::End);
    }

    void updateTitle()
    {
        const QString marker = editor_->document()->isModified() ? "*" : "";
        const QString file =
            currentFile_.isEmpty() ? "Untitled" : QFileInfo(currentFile_).fileName();
        setWindowTitle(QString("%1%2 - %3").arg(file, marker, kAppName));
    }

    void setStatus(const QString& message)
    {
        statusPill_->setText(message);
    }

    QSettings settings_;
    AppTheme theme_;
    QString themePath_;
    QString currentFile_;
    QString projectDirectory_;
    QVector<CompilerInfo> compilers_;
    std::optional<BuildContext> buildContext_;
    std::unique_ptr<QProcess> process_;

    QSplitter* verticalSplitter_ = nullptr;
    QSplitter* topSplitter_ = nullptr;
    QWidget* sidebar_ = nullptr;
    QListWidget* fileList_ = nullptr;
    SourceEditor* editor_ = nullptr;
    PdfPreview* pdfPreview_ = nullptr;
    QTabWidget* bottomTabs_ = nullptr;
    QPlainTextEdit* buildLog_ = nullptr;
    QWidget* terminalHost_ = nullptr;
    QTermWidget* terminal_ = nullptr;
    QComboBox* compilerCombo_ = nullptr;
    QComboBox* buildModeCombo_ = nullptr;
    QToolButton* toggleSidebarButton_ = nullptr;
    QToolButton* togglePdfButton_ = nullptr;
    QLabel* statusPill_ = nullptr;
    int editorFontSize_ = kDefaultEditorFontSize;
    int terminalFontSize_ = kDefaultTerminalFontSize;
    bool autoReloadPdf_ = true;
    bool autoReloadSource_ = true;
    bool diskChangeDialogOpen_ = false;
    QString lastDiskText_;
    QString watchedSourceFile_;
    QFileSystemWatcher* sourceWatcher_ = nullptr;
    QTimer sourceReloadDebounce_;
};

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QApplication::setStyle("Fusion");
    QCoreApplication::setOrganizationName(kOrgName);
    QCoreApplication::setApplicationName(kAppName);
    QCoreApplication::setApplicationVersion(APP_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription(
        "LiTeX is a native minimal LaTeX editor, compiler, PDF previewer, and terminal."
    );
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("file", "TeX file to open.", "[file]");
    parser.process(app);

    MainWindow window;
    window.show();

    const QStringList files = parser.positionalArguments();
    if (!files.isEmpty()) {
        const QString path = QFileInfo(files.first()).absoluteFilePath();
        if (QFileInfo::exists(path)) {
            QTimer::singleShot(0, &window, [&window, path] { window.openInitialFile(path); });
        }
    }

    return app.exec();
}
