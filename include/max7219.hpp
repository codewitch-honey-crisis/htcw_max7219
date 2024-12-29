#pragma once
#if __has_include(<Arduino.h>)
#include <Arduino.h>
#include <SPI.h>

#else
#include "driver/spi_master.h"
#include "driver/gpio.h"
#endif
namespace htcw {
    template<uint8_t WidthSegments,uint8_t HeightSegments,int16_t PinCS = -1>
    struct max7219 final {
        
        static_assert(WidthSegments>0 && HeightSegments>0,"Not enough segments");
        static_assert(WidthSegments*HeightSegments<=256,"Too many segments");
        constexpr static const uint16_t width_segments = WidthSegments;
        constexpr static const uint16_t height_segments = HeightSegments;
        constexpr static const uint8_t segments = WidthSegments*HeightSegments;

        constexpr static const uint16_t width = WidthSegments*8;
        constexpr static const uint16_t height = HeightSegments*8;    
        constexpr static const int16_t pin_cs = PinCS;
    private:
    
        bool m_initialized;
        uint8_t m_frame_buffer[width*height/8];

#ifdef ARDUINO
        SPIClass m_spi;
#else
        spi_host_device_t m_spi_dev;
        spi_device_handle_t m_spi;
#endif
        static inline uint16_t shuffle(uint16_t val)
        {
            return (val >> 8) | (val << 8);
        }
        void spi_write(const uint8_t* data,size_t size) {
#ifdef ARDUINO
            SPISettings settings;
            settings._bitOrder = MSBFIRST;
            settings._clock = 10*1000*1000;
            settings._dataMode = SPI_MODE0;
            if(pin_cs>-1) {
                digitalWrite(pin_cs,LOW);
            }
            m_spi.beginTransaction(settings);
            while(size--) {
                m_spi.write(*(data++));
            }
            m_spi.endTransaction();
            if(pin_cs>-1) {
                digitalWrite(pin_cs,HIGH);
            }
#else
            if(pin_cs>-1) {
                gpio_set_level((gpio_num_t)pin_cs,0);
            }
            spi_transaction_t tran;
            tran.flags = 0;
            tran.tx_buffer = data;
            tran.length = size*8;
            tran.rx_buffer = 0;
            tran.rxlength = 0;
            spi_device_transmit( m_spi,&tran);
            if(pin_cs>-1) {
                gpio_set_level((gpio_num_t)pin_cs,1);
            }
#endif
        }
        
        static bool normalize_values(int&x1,int&y1,int&x2,int&y2,bool check_bounds=true) {
            // normalize values
            int tmp;
            if(x1>x2) {
                tmp=x1;
                x1=x2;
                x2=tmp;
            }
            if(y1>y2) {
                tmp=y1;
                y1=y2;
                y2=tmp;
            }
            if(check_bounds) {
                int w,h;
                w=width;
                h=height;
                if(x1>=w||y1>=h)
                    return false;
                if(x2>=w)
                    x2=w-1;
                if(y2>h)
                    y2=h-1;
            }
            return true;
        }
        
        bool display_update() {
            int line = 0;
            for(int y=0;y<height;y+=8) {
                for(int x = 0;x<width;x+=8) {        
                    for(int yy=0;yy<8;++yy) {
                        int yyy = y + yy;
                        if(x<width&&x+7>=0&&yyy<height&&yyy>=0) {
                            const uint8_t* p = m_frame_buffer+(yyy*width+x)/8;
                            if(!set_line(line,*p)) {
                                return false;
                            }
                        }
                        ++line;
                    }
                }
            }
            return true;
        }
        
        bool disable_decode_mode()
        {
            if(!send( 0xFF, (9 << 8) )) {
                return false;
            }
            return clear();
        }

        bool set_brightness(uint8_t value)
        {
            if(value > 15) {
                return false;
            }

            return send( 0xFF, (10 << 8) | value);

        }

        inline bool set_enabled(bool enabled)
        {
            return send(0xFF, (12 << 8) | enabled);
        }


        bool clear()
        {
            for (uint8_t i = 0; i < 8; i++) {
                if(!send( 0xFF, ((1 << 8) + ((uint16_t)i << 8)))) {
                    return false;
                }
            }
            return true;
        }

        bool send(uint8_t seg, uint16_t value)
        {
      
            uint16_t buf[8] = { 0 };
            value = shuffle(value);
            if (seg == 0xFF)
            {
                for (uint8_t i = 0; i < segments; i++)
                    buf[i] = value;
            }
            else {
                buf[seg] = value;
            }
            spi_write((uint8_t*)buf,segments*2);
            return true;
            
        }
        bool set_line(uint8_t line, uint8_t val)
        {
            if(!initialize()) {
                return false;
            }
            if (line >= segments*8)
            {
                return false;
            }

            uint8_t c = line / 8;
            uint8_t d = line % 8;

            return send(c, ((1 << 8) + ((uint16_t)d << 8)) | val);
        }
        
        max7219(const max7219& rhs)=delete;
        max7219& operator=(const max7219& rhs)=delete;
    public:
#ifdef ARDUINO
        max7219(SPIClass& spi = SPI) : 
            m_initialized(false),m_spi(spi) {
        }
#else
        max7219(spi_host_device_t spi = SPI3_HOST) : 
            m_initialized(false),m_spi_dev(spi) {
        }
#endif
        inline bool initialized() const {
            return m_initialized;
        }
        bool initialize() {
            if(!m_initialized) {
#ifdef ARDUINO
                if(pin_cs>-1) {
                    pinMode(pin_cs,OUTPUT);
                    digitalWrite(pin_cs,HIGH);
                }
                m_spi.begin();
                // no initialization done for ESP-IDF
#else
                spi_device_interface_config_t cfg;
                memset(&cfg,0,sizeof(cfg));
                cfg.clock_speed_hz = 10*1000*1000;
                cfg.queue_size=8;                
                if(ESP_OK!=spi_bus_add_device(m_spi_dev,&cfg,&m_spi)) {
                    return false;
                }
                if(pin_cs>-1) {
                    gpio_set_direction((gpio_num_t)pin_cs,GPIO_MODE_OUTPUT);
                    gpio_set_level((gpio_num_t)pin_cs,1);
                }
#endif
                if(!set_enabled(false)) {
                    return false;
                }
                if(!send(0xFF, (15 << 8))) {
                    return false;
                }
                if(!send(0xFF, (11 << 8) | 7)) {
                    return false;
                }
                if(!disable_decode_mode()) {
                    return false;
                }
                if(!set_brightness(0)) {
                    return false;
                }
                if(!set_enabled(true)) {
                    return false;
                }
                m_initialized=true;
                
            }
            return true;
        }
        uint8_t* frame_buffer() {
            return m_frame_buffer;
        }
        void update() {
            display_update();
        }
    public:
        using type = max7219;
    };
}