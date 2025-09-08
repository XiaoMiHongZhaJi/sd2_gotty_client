import os
import re
import collections
import sys


# 传入一个文件夹，自动统计出现次数最多的前n个中文字符，并转换为 unicode
# 然后使用 Processing 将 unicode 转换为 vlw 文件 (https://processing.org/releases)
# 然后将 vlw 文件 转换为 h 文件 (https://tomeko.net/online_tools/file_to_hex.php)

# 用法: python script.py <文件夹路径> [top_n] [补充文件]
def count_non_english_chars(folder, top_n, other_file=None, exts=("txt", "srt", "ass")):
    counter = collections.Counter()
    # 过滤常见英文和符号
    pattern = re.compile(r"[A-Za-z0-9\s\.,!?;:'\"()\[\]{}<>/\\|@#$%^&*_+=-]")

    for root, _, files in os.walk(folder):
        for file in files:
            if file.lower().endswith(exts):
                filepath = os.path.join(root, file)
                try:
                    with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
                        text = f.read()
                        for ch in text:
                            # 去掉 ASCII 范围英文符号，同时排除 3 字节及以上字符 (ord(ch) > 0xFFFF)
                            if not pattern.match(ch) and ord(ch) <= 0xFFFF:
                                # if ord(ch) > 0xFFFF:
                                counter[ch] += 1
                except Exception as e:
                    print(f"读取 {filepath} 出错: {e}")

    # 取前 top_n 个
    most_common = counter.most_common(top_n)

    # 打印结果
    for char, freq in most_common:
        print(f"{char} : {freq}")

    # 转换成 Unicode 码
    unicode_list = [f"0x{ord(ch):04x}" for ch, _ in most_common]
    print("\nUnicode 列表：")
    print(", ".join(unicode_list))

    # 补充文件中的字符
    if other_file:
        most_common_chars = [char for char, _ in most_common]
        other_counter = collections.Counter()

        filepath = os.path.join("./", other_file)
        try:
            with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
                text = f.read()
                for ch in text:
                    # 去掉 ASCII 范围英文符号，同时排除 3 字节及以上字符 (ord(ch) > 0xFFFF)
                    if not pattern.match(ch) and ord(ch) <= 0xFFFF:
                        if not ch in most_common_chars:
                            other_counter[ch] += 1
        except Exception as e:
            print(f"读取 {other_counter} 出错: {e}")

        # 取前 top_n 个
        other_most_common = other_counter.most_common(top_n)

        print(f"以下是补充字符：" + str(len(other_counter)))
        # 打印结果
        # for char, freq in other_most_common:
        #    print(f"{char} : {freq}")

        # 转换成 Unicode 码
        other_unicode_list = [f"0x{ord(ch):04x}" for ch, _ in other_most_common]
        print("\nUnicode 列表：")
        print(", ".join(other_unicode_list))

        most_common_dict = dict(most_common)
        other_most_common_dict = dict(other_most_common)

        print(most_common_dict.get("。"))
        print(other_most_common_dict.get("。"))


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("用法: python script.py <文件夹路径> [top_n] [补充文件]")
    else:
        folder_path = sys.argv[1]
        top_n = None
        other_file = None
        if len(sys.argv) > 2:
            top_n = int(sys.argv[2])
        if len(sys.argv) > 3:
            other_file = sys.argv[3]
        count_non_english_chars(folder_path, top_n, other_file)
