// clang-format off
// Legend:
// +------+------------+
// | Code | Type       |
// +------+------------+
// |   0  | Path       |
// |   1  | Wall       |
// |   2  | Power Pill |
// |   3  | Ghost Base |
// |   9  | Other      |
// +------+------------+
// UBYTE mapping_stage_0001[320] = {
//     9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
//     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//     1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 2, 1,
//     1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1,
//     1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1,
//     1, 1, 1, 0, 1, 9, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1,
//     1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
//     1, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1,
//     1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1,
//     1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1,
//     1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1,
//     1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1,
//     1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 0, 1,
//     1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 2, 1,
//     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//     9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9
// };
// clang-format on

/*
 * Runtime Binary Loading Example
 * ------------------------------
 * #include <stdio.h>
 *
 * // Allocate the array once (320 bytes)
 * UBYTE current_stage_map[320];
 *
 * // Call this when you want to load a level
 * void load_stage(const char* filepath) {
 *     FILE *file = fopen(filepath, "rb");
 *     if (file != NULL) {
 *         // Read 320 bytes directly into the array
 *         fread(current_stage_map, sizeof(UBYTE), 320, file);
 *         fclose(file);
 *     } else {
 *         printf("Failed to load stage map: %s\n", filepath);
 *     }
 * }
 *
 * // Usage Example:
 * // load_stage("collision/stage-0001.bin");
 */
