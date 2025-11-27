from PIL import Image
import numpy as np

# Load image
img = Image.open('bobby.jpeg').convert('RGB')

# Desired width
width = 100
aspect = img.height / img.width
height = int(aspect * width*0.45)

img_resized = img.resize((width, height))

# ASCII chars by intensity
chars = np.array(list(".:#-+=@~!$%&^*"))

# Compute grayscale
arr = np.array(img_resized)
gray = np.dot(arr[..., :3], [0.2989, 0.5870, 0.1140])
gray_norm = (gray - gray.min()) / (gray.max() - gray.min())
indices = (gray_norm * (len(chars) - 1)).astype(int)

# Build ANSI-colored ASCII
lines = []
for y in range(height):
    line = ""
    for x in range(width):
        r, g, b = arr[y, x]
        char = chars[indices[y, x]]
        line += f"\x1b[38;2;{r};{g};{b}m{char}\x1b[0m"
    lines.append(line)


for i in range(18):
    lines.pop()

lines.pop(0)
lines.pop(1)
lines.pop(2)
lines.pop(3)
ascii_art = "\n".join(lines)
ascii_art += "\n"



print(ascii_art)

# from PIL import Image
# import numpy as np

# img = Image.open("itachi.jpg").convert("RGB")

# # Resize for terminal (40 wide here; you can increase)
# WIDTH = 40
# aspect = img.height / img.width
# HEIGHT = int(aspect * WIDTH)   # doubled because 2 pixels per char
# img = img.resize((WIDTH, HEIGHT))

# # Convert to numpy
# arr = np.array(img)

# # Split into top/bottom pixel pairs
# lines = []
# for y in range(0, HEIGHT, 2):
#     line = ""
#     for x in range(WIDTH):
#         top = arr[y, x]
#         bottom = arr[y+1, x] if y+1 < HEIGHT else top

#         # brightness
#         t_val = top.mean()
#         b_val = bottom.mean()

#         # choose block char
#         if t_val < 128 and b_val < 128:
#             char = "█"
#             color = top
#         elif t_val < 128 and b_val >= 128:
#             char = "▀"
#             color = top
#         elif t_val >= 128 and b_val < 128:
#             char = "▄"
#             color = bottom
#         else:
#             char = " "
#             color = top

#         r,g,b = color
#         line += f"\x1b[38;2;{r};{g};{b}m{char}\x1b[0m"

#     lines.append(line)

# # Save output
# with open("half_block_ascii.txt", "w", encoding="utf-8") as f:
#     f.write("\n".join(lines))


# right_text = [
# "Gather ye rose-buds while ye may,",
# "Old Time is still a-flying;",
# "And this same flower that smiles today",
# "Tomorrow will be dying."
# ]

# import re

# ANSI_ESCAPE = re.compile(r'\x1b\[[0-9;]*m')

# def visible_len(line: str) -> int:
#     no_ansi = ANSI_ESCAPE.sub("", line)
#     return len(no_ansi.rstrip())

# GAP = 3  # how many spaces between art and text
# final_lines = []

# for i, ascii_line in enumerate(lines):
#     txt = right_text[i] if i < len(right_text) else ""

#     # strip trailing spaces from the ASCII part
#     ascii_clean = ascii_line.rstrip()

#     # you don't actually need vis for curvature now,
#     # but you can keep it if you want to inspect widths:
#     # vis = visible_len(ascii_line)

#     final_lines.append(ascii_clean + " " * GAP + txt)

# final_output = "\n".join(final_lines)
# final_output += "\n"


with open("ascii_art.txt", "w", encoding="utf-8") as f:
    f.write(ascii_art)


# mv ascii_art.txt ~/.ascii_art.txt
