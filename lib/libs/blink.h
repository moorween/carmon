#ifndef Blink_H
    #define Blink_H
    #define ORDER_GRB       // порядок цветов ORDER_GRB / ORDER_RGB / ORDER_BRG
    #define COLOR_DEBTH 2  
    enum COLORS {
        WHITE =		0xFFFFFF,	// белый
        SILVER =	0xC0C0C0,	// серебро
        GRAY =		0x808080,	// серый
        BLACK =		0x000000,	// чёрный	
        RED =		0xFF0000,	// красный
        MAROON =	0x800000,	// бордовый
        ORANGE =	0xFF3000,	// оранжевый
        YELLOW =	0xFF8000,	// жёлтый
        OLIVE =		0x808000,	// олива
        LIME =		0x00FF00,	// лайм
        GREEN =		0x008000,	// зелёный
        AQUA =		0x00FFFF,	// аква
        TEAL =		0x008080,	// цвет головы утки чирка
        BLUE =		0x0000FF,	// голубой
        NAVY =		0x000080,	// тёмно-синий
        MAGENTA =	0xFF00FF,	// розовый
        PURPLE =	0x800080,	// пурпурный
    };    
#endif

class Blinker {
        public:
	        Blinker();
	        void tick();	
            void blink();
            void blink(int count, COLORS color);
        private:
            COLORS _color = BLACK;
            int _counts = 0; 
    };