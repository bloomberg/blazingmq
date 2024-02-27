import os

bmqa_directory = "bmqa"
z_bmqa_directory = "z_bmqa"

for filename in os.listdir(bmqa_directory):
    if filename.endswith(".h"):
        with open(os.path.join(bmqa_directory, filename), "r") as bmqa_file:
            lines = bmqa_file.readlines()
            docstring_lines = []
            copy_docstring = False
            for line in lines:
                if "@PURPOSE" in line:
                    docstring_lines.append(line)
                    copy_docstring = True
                elif copy_docstring and line.strip() == "":
                    break
                elif copy_docstring:
                    docstring_lines.append(line)

        with open(os.path.join(z_bmqa_directory, "z_" + filename), "r+") as z_bmqa_file:
            lines = z_bmqa_file.readlines()
            z_bmqa_file.seek(0)
            z_bmqa_file.truncate()
            include_found = False
            copied = False

            for line in lines:
                print("line:", line, end="")
                if line.startswith("#include"):
                    z_bmqa_file.write(line)
                    z_bmqa_file.write("\n")
                    include_found = True
                elif include_found and not copied:
                    z_bmqa_file.writelines(docstring_lines)
                    z_bmqa_file.write(line)
                    include_found = False
                    copied = True
                else:
                    z_bmqa_file.write(line)
