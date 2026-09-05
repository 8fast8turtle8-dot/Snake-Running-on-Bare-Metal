extern "C" void start() {
    volatile unsigned short* vga = (unsigned short*)0xB8000;
    const int width  = 80;
    const int height = 25;

    unsigned short blank = (0x0F << 8) | ' ';

    struct Segment {
        int x;
        int y;
    };

    while (1) { // outer loop: allows retry on Enter
        // Clear screen
        for (int i = 0; i < width * height; ++i) {
            vga[i] = blank;
        }

        // ---- Snake state ----
        Segment snake[64];
        int snakeLength = 5;
        int direction   = 3;

        int startX = width / 2;
        int startY = height / 2;
        for (int i = 0; i < snakeLength; ++i) {
            snake[i].x = startX - i;
            snake[i].y = startY;
        }

        // TRUE RANDOM SEEDING
        unsigned short tick = 0;   // now 16-bit

        // initial food
        unsigned int seed = 123;
        seed = seed * 1103515245 + 12345;
        int foodX = (seed >> 16) % width;
        int foodY = (seed >> 8)  % height;

        int score = 0;

        bool extended = false;
        bool running  = true;

        // ---- main game loop ----
        while (running) {
            // --- keyboard: PS/2 arrow keys ---
            for (int k = 0; k < 32; ++k) {
                unsigned char status;
                asm volatile("inb %1, %0" : "=a"(status) : "Nd"(0x64));
                if (!(status & 0x01)) continue;

                unsigned char scancode;
                asm volatile("inb %1, %0" : "=a"(scancode) : "Nd"(0x60));

                if (scancode == 0xE0) {
                    extended = true;
                    continue;
                }

                if (extended) {
                    if (scancode == 0x48 && direction != 1) direction = 0;
                    else if (scancode == 0x50 && direction != 0) direction = 1;
                    else if (scancode == 0x4B && direction != 3) direction = 2;
                    else if (scancode == 0x4D && direction != 2) direction = 3;
                    extended = false;
                }
            }

            // --- game tick ---
            Segment head = snake[0];

            if (direction == 0)      head.y -= 1;
            else if (direction == 1) head.y += 1;
            else if (direction == 2) head.x -= 1;
            else if (direction == 3) head.x += 1;

            if (head.x < 0 || head.x >= width || head.y < 0 || head.y >= height)
                running = false;

            for (int i = 0; i < snakeLength; ++i)
                if (snake[i].x == head.x && snake[i].y == head.y)
                    running = false;

            if (!running) break;

            for (int i = snakeLength - 1; i > 0; --i)
                snake[i] = snake[i - 1];
            snake[0] = head;

            // --- food collision ---
            if (head.x == foodX && head.y == foodY) {
                if (snakeLength < 64) {
                    snake[snakeLength] = snake[snakeLength - 1];
                    snakeLength++;
                }
                score++;

                // TRUE RANDOM FOOD SPAWN (16-bit)
                seed = (seed ^ tick) * 1103515245 + 12345;

                foodX = (seed >> 16) % width;
                foodY = (seed >> 8)  % height;

                // ensure food not inside snake
                bool bad = true;
                while (bad) {
                    bad = false;
                    for (int i = 0; i < snakeLength; ++i) {
                        if (snake[i].x == foodX && snake[i].y == foodY) {
                            seed = seed * 1103515245 + 12345;
                            foodX = (seed >> 16) % width;
                            foodY = (seed >> 8)  % height;
                            bad = true;
                            break;
                        }
                    }
                }
            }

            // --- draw board ---
            for (int i = 0; i < width * height; ++i)
                vga[i] = blank;

            vga[foodY * width + foodX] = (0x0A << 8) | '@';

            for (int i = 0; i < snakeLength; ++i) {
                int idx = snake[i].y * width + snake[i].x;
                char c = (i == 0) ? 'O' : 'o';
                vga[idx] = (0x0E << 8) | c;
            }

            // score
            const char* label = "Score: ";
            for (int i = 0; i < 7; ++i)
                vga[i] = (0x0F << 8) | label[i];

            char buf[6];
            int n = score;
            int pos = 0;
            if (n == 0) buf[pos++] = '0';
            else {
                char tmp[6];
                int tpos = 0;
                while (n > 0) {
                    tmp[tpos++] = '0' + (n % 10);
                    n /= 10;
                }
                while (tpos > 0) buf[pos++] = tmp[--tpos];
            }
            for (int i = 0; i < pos; ++i)
                vga[7 + i] = (0x0F << 8) | buf[i];

            // delay + tick
            for (int d = 0; d < 20000000; ++d) {
                asm volatile("nop");
                tick++;
            }
        }

        // ---- game over ----
        for (int i = 0; i < width * height; ++i)
            vga[i] = blank;

        const char* msg = "GAME OVER - Press Enter to retry";
        int len = 32;
        int x0  = (width - len) / 2;
        int y0  = height / 2;
        for (int i = 0; i < len; ++i)
            vga[y0 * width + (x0 + i)] = (0x0C << 8) | msg[i];

        // wait for Enter
        bool waiting = true;
        while (waiting) {
            unsigned char status;
            asm volatile("inb %1, %0" : "=a"(status) : "Nd"(0x64));
            if (!(status & 0x01)) continue;

            unsigned char scancode;
            asm volatile("inb %1, %0" : "=a"(scancode) : "Nd"(0x60));

            if (scancode == 0x1C) waiting = false;
        }
    }
}

