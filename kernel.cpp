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
        int direction   = 3;        // 0=UP,1=DOWN,2=LEFT,3=RIGHT

        int startX = width / 2;
        int startY = height / 2;
        for (int i = 0; i < snakeLength; ++i) {
            snake[i].x = startX - i;
            snake[i].y = startY;
        }

        int foodX = 10;
        int foodY = 10;

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
                    if (scancode == 0x48 && direction != 1) direction = 0; // Up
                    else if (scancode == 0x50 && direction != 0) direction = 1; // Down
                    else if (scancode == 0x4B && direction != 3) direction = 2; // Left
                    else if (scancode == 0x4D && direction != 2) direction = 3; // Right
                    extended = false;
                }
            }

            // --- game tick: move snake, check collisions ---
            Segment head = snake[0];

            if (direction == 0)      head.y -= 1; // UP
            else if (direction == 1) head.y += 1; // DOWN
            else if (direction == 2) head.x -= 1; // LEFT
            else if (direction == 3) head.x += 1; // RIGHT

            // wall collision
            if (head.x < 0 || head.x >= width || head.y < 0 || head.y >= height) {
                running = false;
            }

            // self collision
            for (int i = 0; i < snakeLength; ++i) {
                if (snake[i].x == head.x && snake[i].y == head.y) {
                    running = false;
                }
            }

            if (!running) break;

            // shift body
            for (int i = snakeLength - 1; i > 0; --i) {
                snake[i] = snake[i - 1];
            }
            snake[0] = head;

            // food collision
            if (head.x == foodX && head.y == foodY) {
                if (snakeLength < 64) {
                    snake[snakeLength] = snake[snakeLength - 1];
                    snakeLength++;
                }
                score++; // increment score

                // super simple "random": move food
                foodX += 3;
                foodY += 1;
                if (foodX >= width)  foodX -= width;
                if (foodY >= height) foodY -= height;
            }

            // --- draw board ---
            for (int i = 0; i < width * height; ++i) {
                vga[i] = blank;
            }

            // draw food
            if (foodX >= 0 && foodX < width && foodY >= 0 && foodY < height) {
                int idx = foodY * width + foodX;
                vga[idx] = (0x0A << 8) | '@';
            }

            // draw snake
            for (int i = 0; i < snakeLength; ++i) {
                int sx = snake[i].x;
                int sy = snake[i].y;
                if (sx < 0 || sx >= width || sy < 0 || sy >= height) continue;
                int idx = sy * width + sx;
                char c = (i == 0) ? 'O' : 'o';
                vga[idx] = (0x0E << 8) | c;
            }

            // draw score in top-left: "Score: [number]"
            const char* label = "Score: ";
            int labelLen = 7;
            for (int i = 0; i < labelLen; ++i) {
                vga[i] = (0x0F << 8) | label[i];
            }

            // convert score to decimal (max a few digits)
            char buf[6];
            int n = score;
            int pos = 0;
            if (n == 0) {
                buf[pos++] = '0';
            } else {
                char tmp[6];
                int tpos = 0;
                while (n > 0 && tpos < 6) {
                    tmp[tpos++] = '0' + (n % 10);
                    n /= 10;
                }
                // reverse into buf
                while (tpos > 0) {
                    buf[pos++] = tmp[--tpos];
                }
            }
            // write score digits after "Score: "
            for (int i = 0; i < pos; ++i) {
                vga[labelLen + i] = (0x0F << 8) | buf[i];
            }

            // crude delay
            for (int d = 0; d < 20000000; ++d) {
                asm volatile("nop");
            }
        }

        // ---- game over screen ----
        for (int i = 0; i < width * height; ++i) {
            vga[i] = blank;
        }

        const char* msg = "GAME OVER - Press Enter to retry";
        int len = 32;
        int x0  = (width - len) / 2;
        int y0  = height / 2;
        for (int i = 0; i < len; ++i) {
            int idx = y0 * width + (x0 + i);
            vga[idx] = (0x0C << 8) | msg[i];
        }

        // wait for Enter (make code 0x1C)
        bool waiting = true;
        while (waiting) {
            unsigned char status;
            asm volatile("inb %1, %0" : "=a"(status) : "Nd"(0x64));
            if (!(status & 0x01)) continue;

            unsigned char scancode;
            asm volatile("inb %1, %0" : "=a"(scancode) : "Nd"(0x60));

            if (scancode == 0x1C) { // Enter pressed
                waiting = false; // restart outer loop
            }
        }
    }
}


