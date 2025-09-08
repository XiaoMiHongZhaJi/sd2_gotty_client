#ifndef TFT_TERMINAL_H
#define TFT_TERMINAL_H

#include <TFT_eSPI.h>
#include "font/top900.h"
// #include "font/top800.h"

// top900.h 字体中等
#define MAX_LINES_CAP 9    // 期望显示的行数
#define MAX_LINEHEIGHT 26  // 行高 = 240 / MAX_LINES_CAP
#define FONT_SIZE 3        // 默认字体大小 仅针对tft默认字体有效

// top800.h 字体较小
// #define MAX_LINES_CAP 12
// #define MAX_LINEHEIGHT 20
// #define FONT_SIZE 2

class TFTTerminal {
public:
  TFTTerminal(TFT_eSPI &display)
    : tft(display), maxLines(MAX_LINES_CAP), lineHeight(MAX_LINEHEIGHT) {
    clear();
  }

  void tft_init() {
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(FONT_SIZE);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.loadFont(top_char);
    tft.setCursor(0, 0);
  }

  void clear() {
    for (int i = 0; i < MAX_LINES_CAP; i++) lines[i] = "";
    tft.fillScreen(TFT_BLACK);
  }

  String getNextUTF8Char(const String &s, int &index) {
    unsigned char c = s[index];
    int len = 1;
    if (c >= 0xF0) len = 4;       // 4字节 UTF-8
    else if (c >= 0xE0) len = 3;  // 3字节 UTF-8（中文常用）
    else if (c >= 0xC0) len = 2;  // 2字节 UTF-8
    else len = 1;                 // ASCII
    String ch = s.substring(index, index + len);
    index += len;
    return ch;
  }

  // 替换方框时被误伤的字符
  const String whitelist = "。；‘·\"]{}*)`";
  void pushWrappedLines(const String &text, int color) {
    String line = "";
    int i = 0;
    while (i < text.length()) {
      String ch = getNextUTF8Char(text, i);
      // 将字库中找不到的字的方框替换为_，方框的长度需自行测试替换
      // Serial.println(tft.textWidth(ch));
      if (tft.textWidth(ch) == 7 && whitelist.indexOf(ch) == -1) {
        // Serial.println("已跳过字符: " + ch);
        ch = "_";
      }
      int w = tft.textWidth(line + ch);
      if (w > tft.width()) {
        pushLine(line, false, color);
        line = ch;
      } else {
        line += ch;
      }
    }

    if (line.length() > 0) pushLine(line, false, color);
  }

  void println(const String &text) {
    int color = colors[colorIndex];
    colorIndex = (colorIndex + 1) % colorCount;
    // 按 \n 先分割成多行
    int start = 0;
    int idx;
    while ((idx = text.indexOf('\n', start)) != -1) {
      String line = text.substring(start, idx);
      pushWrappedLines(line, color);  // 处理每行的自动换行
      start = idx + 1;
    }
    // 处理最后一行（或没有换行符的情况）
    String lastLine = text.substring(start);
    pushWrappedLines(lastLine, color);

    redraw();  // 一次性刷新
  }

  // 打印字符串，自动替换 emoji
  void printWithEmoji(const String &text) {
    String replaced = replaceEmoji(text);
    println(replaced);
  }

private:
  TFT_eSPI &tft;
  String lines[MAX_LINES_CAP];
  int lineColors[MAX_LINES_CAP] = { TFT_WHITE };  // 每行对应颜色
  int maxLines;
  int lineHeight;

  // 循环字体颜色数组
  const int colors[8] = {
    TFT_WHITE, TFT_CYAN, TFT_GREEN, TFT_YELLOW,
    TFT_MAGENTA, TFT_ORANGE, TFT_RED, TFT_BLUE
  };
  const int colorCount = 8;
  int colorIndex;

  void pushLine(const String &line, bool refresh = true, int color = TFT_WHITE) {
    // 上移数组，丢掉最旧行
    for (int i = 0; i < maxLines - 1; i++) {
      lines[i] = lines[i + 1];
      lineColors[i] = lineColors[i + 1];
    }
    lines[maxLines - 1] = line;
    lineColors[maxLines - 1] = color;
    if (refresh) redraw();
  }

  void redraw() {
    tft.fillScreen(TFT_BLACK);
    for (int i = 0; i < maxLines; i++) {
      tft.setCursor(0, i * lineHeight);
      tft.setTextColor(lineColors[i], TFT_BLACK);
      tft.print(lines[i]);
    }
  }

  String replaceEmoji(const String &text) {
    String result = text;
    for (int i = 0; i < emojiCount; i++) {
      result.replace(emojiTable[i].emoji, emojiTable[i].text);
    }
    return result;
  }

public:
  struct EmojiMap {
    const char *emoji;
    const char *text;
  };
  static const EmojiMap emojiTable[];
  static const int emojiCount;
};

// ===================== Emoji 表 =====================
const TFTTerminal::EmojiMap TFTTerminal::emojiTable[] = {

  { "👤", "[人]" },
  { "😁", "[笑]" },
  { "🔗", "[链接]" },
  { "🤔", "[思考]" },
  { "🌚", "[黑月]" },
  { "🪵", "[木头]" },
  { "🍊", "[orange]" },
  { "🕒", "[时钟]" },
  { "😭", "[cry]" },
  { "🍋", "[lemon]" },
  { "🥰", "[爱心笑]" },
  { "🤖", "[机器人]" },
  { "😆", "[大笑]" },
  { "🎉", "[tada]" },
  { "🍪", "[饼干]" },
  { "🌸", "[花]" },
  { "🤣", "[笑翻]" },
  { "😂", "[laugh_cry]" },
  { "🐏", "[ram]" },
  { "😈", "[smiling_devil]" },
  { "😅", "[黄豆]" },
  { "🕊", "[dove]" },
  { "📝", "[记录]" },
  { "🙈", "[see_no_evil]" },
  { "🫡", "[salute]" },
  { "🍠", "[地瓜]" },
  { "👀", "[看]" },
  { "😇", "[天使]" },
  { "🤡", "[clown]" },
  { "🔍", "[magnifying_glass]" },
  { "🤓", "[nerd]" },
  { "👉", "[指向]" },
  { "🥲", "[smile_tear]" },
  { "🙏", "[pray]" },
  { "🔥", "[火]" },
  { "😎", "[sunglasses]" },
  { "🥳", "[派对]" },
  { "😄", "[笑脸]" },
  { "🐶", "[狗]" },
  { "😢", "[伤心]" },
  { "🥹", "[感动]" },
  { "👮", "[警察]" },
  { "🧐", "[思考脸]" },
  { "🙋", "[举手]" },
  { "🀄", "[中]" },
  { "🥴", "[woozy]" },
  { "🤨", "[raised_eyebrow]" },
  { "👍", "[赞]" },
  { "😱", "[scream]" },
  { "😡", "[生气]" },
  { "😨", "[害怕]" },
  { "🫠", "[melting_face]" },
  { "😳", "[flushed]" },
  { "🤯", "[爆炸]" },
};

const int TFTTerminal::emojiCount = sizeof(TFTTerminal::emojiTable) / sizeof(TFTTerminal::emojiTable[0]);

#endif
