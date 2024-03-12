import re


non_functions = set(["if", "else if", "else", "for", "while", "do"])


def append_to_file(string, filename):
    try:
        with open(filename, 'a') as file:
            file.write(string)
    except IOError:
        print("Error: Unable to append string to file.")


def extract_words(lines): #we need to change this function to find the keywords after '(' because of templated types such as <int, int> 
    # Regular expression to match words followed by ',' or ')'
    # \b asserts position at a word boundary
    # \w+ matches one or more word characters (a-z, A-Z, 0-9, _)
    # (?=[,)]) is a positive lookahead to ensure the word is followed by ',' or ')'
    pattern = r'\b\w+(?=[,)])'

    extracted_words = set()  # Use a set to avoid duplicate words

    for line in lines:
        matches = re.findall(pattern, line)
        extracted_words.update(matches)  # Add found words to the set

    return list(extracted_words)  # Convert set to list before returning


def process_cpp_file(file_path):
    buffer = []  # to store lines temporarily
    current_function = ""  # to keep track of the current function
    function_descriptors = {}

    with open(file_path, 'r') as file:
        for line in file:
            line = line.strip()  # remove leading/trailing whitespaces

            # Clear buffer if line is '}' or empty
            if line == '}' or line == '':
                buffer.clear()
            else:
                buffer.append(line)  # add line to buffer

                # Update current_function if line contains '{'
                if '{' in line:
                    previous_func = current_function
                    keyword_found = False
                    for buf_line in buffer:
                        if '(' in buf_line:
                            # Extract function name
                            func_name = buf_line.split('(')[0].split()[-1]
                            if func_name not in non_functions:
                                # print(current_function)
                                current_function = func_name
                                function_descriptors[current_function] = extract_words(
                                    buffer)
                                break
                            else:
                                keyword_found = True
                    if previous_func == current_function and not keyword_found:
                        print(
                            f"Function name could not be extracted for buffer: {buffer}")
    print(function_descriptors)
    return function_descriptors
  
def write_commented_file(file_path, new_file_name, function_descriptors):
    buffer = []  # to store lines temporarily
    current_function = ""  # to keep track of the current function
    # print(fd['z_bmqt_CorrelationId__delete'])

    with open(file_path, 'r') as file:
        for line in file:
            temp_line = line
            line = line.strip()  # remove leading/trailing whitespaces

            # Clear buffer if line is '}' or empty
            if line == '}' or line == '':
                for p in buffer:
                    append_to_file(p, new_file_name)
                append_to_file(temp_line, new_file_name)
                buffer.clear()
            else:
                buffer.append(temp_line)  # add line to buffer

                # Update current_function if line contains '{'
                if '{' in line:
                    previous_func = current_function
                    keyword_found = False
                    for buf_line in buffer:
                        if '(' in buf_line:
                            # Extract function name
                            func_name = buf_line.split('(')[0].split()[-1]
                            if func_name not in non_functions:
                                current_function = func_name
                                append_to_file(str(function_descriptors[current_function]), new_file_name)
                                break
                            else:
                                keyword_found = True
                        else:
                            append_to_file(buf_line, new_file_name)

                    if previous_func == current_function and not keyword_found:
                        print(
                            f"Function name could not be extracted for buffer: {buffer}")

# Example of how to use the function
file_path = 'test_file.cpp'
fd = process_cpp_file(file_path)
print("-----------------------------")
print(fd)
print()
write_commented_file(file_path, "output.cpp", fd)
