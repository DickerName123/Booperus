#ifndef FILE_UTILS_H
#define FILE_UTILS_H

int tee(const char *output_file);
int reverse_file(const char *input_file, const char *output_file);
int reverse_file_optimized(const char *input_file, const char *output_file);

#endif // FILE_UTILS_H
