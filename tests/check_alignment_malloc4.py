from string import atoi
with open("c.txt", "r") as f:
  for line in f:
    stripped_line = line.strip()
    if atoi(stripped_line)%8 != 0:
        print(stripped_line + "%8 != 0")
