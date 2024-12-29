#define MAX_CS 5
#if __has_include(<Arduino.h>)
#include <Arduino.h>
#else
#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/spi_master.h>
uint32_t millis() {
    return pdTICKS_TO_MS(xTaskGetTickCount());
}
#endif
#include <gfx.hpp>
#include <max7219.hpp>
// a windows 3.1 font (.FON) file embedded as a header:
// generated with https://honeythecodewitch.com/gfx/converter
#define VGA_8X8_IMPLEMENTATION
#include "assets/vga_8x8.h"

using namespace gfx;
using namespace htcw;
// 4 segments long, 1 segment high, cs pin is 5
using max_t = max7219<4,1,MAX_CS> ;
max_t panel;
using color_t = color<gsc_pixel<1>>; // max pixel type is monochrome
// load the font
const_buffer_stream font_stream(vga_8x8,sizeof(vga_8x8));
win_font fnt(font_stream);
// declare the frame buffer
bitmap<gsc_pixel<1>> frame_buffer;
//gfx::win_font fnt(&font_stream);

void setup() {
    // create the frame_buffer bitmap
    // initialize the font, and the max7219:
    fnt.initialize();
    panel.initialize();
    frame_buffer = bitmap<gsc_pixel<1>>({max_t::width, max_t::height},panel.frame_buffer());
    // clear the bitmap
    draw::filled_rectangle(frame_buffer,frame_buffer.bounds(),color_t::black);
    
    // prepare a text_info struct:
    text_info ti;
    ti.text_font = &fnt;
    ti.text_sz("esp");
    size16 tsz;
    // center the text
    ti.text_font->measure(-1,ti,&tsz);
    srect16 sr = ((srect16)tsz.bounds()).center_inplace((srect16)frame_buffer.bounds());
    // draw the text to the bitmap
    draw::text(frame_buffer,sr,ti,color_t::white);
    // update the display
    panel.update();
}

void loop() {
    static uint32_t ms = 0;
    if(millis()>ms+100) {
        ms = millis();
        for(int y = 0;y<frame_buffer.dimensions().height;++y) {
            auto first = frame_buffer.point(point16(0,y));
            for(int x = 1;x<frame_buffer.dimensions().width;++x) {
                point16 pt(x,y);
                auto px = frame_buffer.point(pt);
                pt.offset_inplace(-1,0);
                frame_buffer.point(pt,px);
            }
            frame_buffer.point(point16(frame_buffer.bounds().x2,y),first);
        }
        panel.update();   
    }
}
#ifndef ARDUINO
extern "C" void app_main() {
    spi_bus_config_t cfg;
    memset(&cfg,0,sizeof(cfg));
    cfg.mosi_io_num=23;
    cfg.sclk_io_num=18;
    cfg.max_transfer_sz = 32*1024+8;
    spi_bus_initialize(SPI3_HOST,&cfg,SPI_DMA_CH_AUTO);
    setup();
    while(1) {
        loop();
        vTaskDelay(5);
    }
}
#endif