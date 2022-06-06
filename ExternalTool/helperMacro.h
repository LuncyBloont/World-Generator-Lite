#ifndef HELPERMACRO_H
#define HELPERMACRO_H

#define DEBUG

#ifdef DEBUG
#define DBPRINT(name, format) printf("[debug] " #name " = " #format " [at '%s' -> '%s(...)': line %d]\n", name, __FILE__, __FUNCTION__, __LINE__)
#define DBAREA(...) do { __VA_ARGS__ } while (0)
#else
#define DBPRINT(name, format) do {} while (0)
#define DBAREA(...) do {} while (0)
#endif

#define VK(...) if (__VA_ARGS__ != VK_SUCCESS) { perror("Failed at " #__VA_ARGS__); exit(-1); }

#endif // HELPERMACRO_H
