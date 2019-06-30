/* Created By: Justin Meiners (2013) */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_WRITE_NO_STDIO
#include "stb_image_write.h" /* http://nothings.org/stb/stb_image_write.h */

#define STB_TRUETYPE_IMPLEMENTATION 
#include "stb_truetype.h" /* http://nothings.org/stb/stb_truetype.h */

#include "cmunrm.h"

void invert_image(unsigned char* buffer, int w, int h)
{
    int N = w * h;
    for (int i = 0; i < N; ++i)
        buffer[i] = 255 - buffer[i];
}

void render_text(stbtt_fontinfo* font_info,
                 unsigned char* bitmap,
                 int* w,
                 float scale,
                 const char* text,
                 size_t text_size)
                
{
    int ascent, descent, line_gap;
    stbtt_GetFontVMetrics(font_info, &ascent, &descent, &line_gap);
    
    ascent *= scale;
    descent *= scale;

    int x = 0;
    for (int i = 0; i < text_size; ++i)
    {
        if (iscntrl(text[i])) continue;

        int c_x1, c_y1, c_x2, c_y2;
        stbtt_GetCodepointBitmapBox(font_info, text[i], scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);
        
        int y = ascent + c_y1;
        
        /* render character (stride and offset is important here) */
        if (bitmap)
        {
            int offset = x + (y * (*w));
            stbtt_MakeCodepointBitmap(font_info, bitmap + offset, c_x2 - c_x1, c_y2 - c_y1, *w, scale, scale, text[i]);
        }
       
        int ax;
        stbtt_GetCodepointHMetrics(font_info, text[i], &ax, 0);
        x += ax * scale;
        
        int kern = stbtt_GetCodepointKernAdvance(font_info, text[i], text[i + 1]);
        x += kern * scale;
    }

    *w = x;
}

unsigned char* read_font_file(const char* path)
{
    FILE* font_file = fopen("font/cmunrm.ttf", "rb");

    if (!font_file)
    {
        fprintf(stderr, "cannot find font: %s\n", path);
        exit(1);
    }

    /* get length of file */
    fseek(font_file, 0, SEEK_END);
    size_t file_size = ftell(font_file);
    fseek(font_file, 0, SEEK_SET);
    
    /* allocate buffer*/
    unsigned char* font_buffer = malloc(file_size);

    if (!font_buffer)
    {
        fprintf(stderr, "allocation failed. out of memory?");
        exit(2);
    }
   
    /* read into buffer */ 
    fread(font_buffer, file_size, 1, font_file);
    fclose(font_file);
    return font_buffer;
}

void show_usage(int help)
{
    const char* usage = "tex2image --out OUTPUT --height HEIGHT --font TTF --invert\n";

    if (help)
    {
        fputs(usage, stdout);
        fputs("--out\tspecify an output file instead of stdout.\n", stdout);
        fputs("--height\tspecify height of text in pixels.\n", stdout);
        fputs("--font\tprovide an alternative font file in TrueType format. (.ttf)\n", stdout);
        fputs("--invert\tdraw white text on black background.\n", stdout);
        fputs("--help\tshow the help text.\n", stdout);
        exit(0);
    }
    else
    {
        fputs(usage, stderr);
        exit(1);
    }
}

static void image_stdio_write(void *context, void *data, int size)
{
    fwrite(data, 1, size, (FILE*)context);
}

int main(int argc, const char * argv[])
{
    unsigned char* font_buffer = font_cmunrm_ttf;
    int line_height = 48;
    int invert = 0;
    const char* out_path = NULL;
    const char* font_path = NULL;

    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "--height") == 0)
        {
            line_height = atoi(argv[i + 1]);
            ++i;
        }
        else if (strcmp(argv[i], "--font") == 0)
        {
            font_path  = argv[i + 1];
            ++i;
        }
        else if (strcmp(argv[i], "--out") == 0)
        {
            out_path = argv[i + 1];
            ++i;
        }
        else if (strcmp(argv[i], "--invert") == 0)
        {
            invert = 1;
        }
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h"))
        {
            show_usage(1);
        }
        else
        {
            show_usage(0);
        }
    }

    if (font_path)
    {
        font_buffer = read_font_file(font_path);
    }

    /* prepare font */
    stbtt_fontinfo font_info;
    if (!stbtt_InitFont(&font_info, font_buffer, 0))
    {
        fprintf(stderr, "failed to initialize font.");
        exit(2);
    }

    /* read stdin into a string.
       we have to do this, because it needs two passes
    */
    size_t text_size = 0;
    size_t text_capacity = 2048;
    char* text = malloc(text_capacity);

    if (!text)
    {
        fprintf(stderr, "allocation failed. out of memory?");
        exit(2);
    }

    while (!feof(stdin))
    {
        if (text_size >= text_capacity)
        {
            text_capacity = text_capacity * 3 / 2;
            text = realloc(text, sizeof(char) * text_capacity);

            if (!text)
            {
                fprintf(stderr, "allocation failed. out of memory?");
                exit(2);
            }
        }

        size_t read = fread(text, sizeof(char), (text_capacity - text_size), stdin);
        text_size += read;
    }
    
    /* cleanup characters */

    for (int i = 0; i < text_size; ++i)
    {
        if (text[i] == '\n')
        {
            text[i] = ' ';
        }
        else if (text[i] == '\r')
        {
            text[i] = ' ';
        }
    }

    float scale = stbtt_ScaleForPixelHeight(&font_info, line_height);

    /* first pass */
    int image_w;  
    int image_h = line_height;

    render_text(&font_info, NULL, &image_w, scale, text, text_size);

    /* second pass */
    unsigned char* bitmap = malloc(image_w * image_h);
    if (!bitmap)
    {
        fprintf(stderr, "allocation failed. out of memory?");
        exit(2);
    }

    render_text(&font_info, bitmap, &image_w, scale, text, text_size);
    
    if (!invert)
    {
        invert_image(bitmap, image_w, image_h);
    }
    
    /* save out a 1 channel image */

    FILE* out_file = stdout;
    if (out_path)
    {
        out_file = fopen(out_path, "wb");
    }

    stbi_write_png_to_func(image_stdio_write, out_file, image_w, image_h, 1, bitmap, image_w);

    if (out_file != stdout)
    {
        fclose(out_file);
    }
    
    if (font_buffer != font_cmunrm_ttf)
    {
        free(font_buffer);
    }

    free(text);
    free(bitmap);
    
    return 0;
}

