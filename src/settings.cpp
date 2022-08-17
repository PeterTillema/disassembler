#include "settings.h"
#include "zdis/zdis.h"

#include <graphx.h>
#include <keypadc.h>
#include <ti/getcsc.h>

const char *setting_strings[][2] = {
        {"Immediate suffix", "No immediate suffix"},
        {"Decimal immediate", "No decimal immediate"},
        {"Mnemonic space", "Mnemonic tab"},
        {"Argument space", "No argument space"},
        {"Compute relative", "Don't compute relative"},
        {"Compute absolute", "Don't compute absolute"}
};

struct zdis_ctx *settings(struct zdis_ctx *ctx) {
    unsigned int row = 0;
    uint8_t key;

    while ((key = os_GetCSC()) != sk_Enter && key != sk_Clear) {
        kb_Scan();
        gfx_FillScreen(222);
        gfx_SetColor(255);
        gfx_FillRectangle_NoClip(60, 90, 200, 60);
        gfx_SetColor(0);
        gfx_SetTextFGColor(0);
        gfx_SetTextBGColor(222);
        gfx_PrintStringXY("Disassembler - by Peter \"PT_\" Tillema", 35, 1);
        gfx_SetTextBGColor(255);

        for (uint8_t i = 0; i < 6; i++) {
            if (row == i) {
                gfx_SetTextFGColor(255);
                gfx_SetTextBGColor(0);
                gfx_FillRectangle_NoClip(60, 90 + 10 * i, 200, 10);
                gfx_PrintStringXY("<", 60, 91 + 10 * i);
                gfx_PrintStringXY(">", 253, 91 + 10 * i);
            }

            // Print the right string
            const char *string = (ctx->zdis_user_size & (1 << i)) ? setting_strings[i][0] : setting_strings[i][1];
            gfx_PrintStringXY(string, (GFX_LCD_WIDTH - gfx_GetStringWidth(string)) / 2, 92 + i * 10);

            if (row == i) {
                gfx_SetTextFGColor(0);
                gfx_SetTextBGColor(255);
            }
        }

        gfx_SwapDraw();

        if (key == sk_Up && row) row--;
        if (key == sk_Down && row < 5) row++;
        if (key == sk_Left || key == sk_Right) {
            if (ctx->zdis_user_size & (1 << row)) {
                ctx->zdis_user_size &= ~(1 << row);
            } else {
                ctx->zdis_user_size |= (1 << row);
            }
        }
    }

    return ctx;
}
