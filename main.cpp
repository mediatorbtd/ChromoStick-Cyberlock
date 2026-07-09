#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

namespace {

constexpr uint8_t kRedPin = PB2;
constexpr uint8_t kGreenPin = PB1;
constexpr uint8_t kBluePin = PB0;
constexpr uint8_t kBuzzerPin = PB3;
constexpr uint8_t kJoystickSwitchPin = PD6;
constexpr uint8_t kJoystickXPin = 0;
constexpr uint8_t kJoystickYPin = 1;

constexpr uint8_t kLcdRsPin = PB4;
constexpr uint8_t kLcdEnablePin = PD7;
constexpr uint8_t kLcdD4Pin = PD5;
constexpr uint8_t kLcdD5Pin = PD4;
constexpr uint8_t kLcdD6Pin = PD3;
constexpr uint8_t kLcdD7Pin = PD2;

volatile uint32_t g_millis = 0;

struct UserCredential {
    const char* user_id;
    const char* passchromo;
};

struct AuthState {
    char input_code[16] = {0};
    uint8_t input_length = 0;
    uint8_t consecutive_failures = 0;
    bool neutral = true;
    bool tamper_flag = false;
    uint32_t attempt_start_ms = 0U;
    uint16_t event_counter = 1000U;
};

constexpr UserCredential users[] = {
    {"EMP001", "RGWBWRGB"},
    {"EMP002", "WRGBRGBW"},
    {"EMP003", "GBRWWRGB"}
};

constexpr uint8_t kUserCount = 3U;
constexpr uint8_t kSecretLength = 8U;
constexpr uint16_t kInstructionDisplayMs = 3000U;
constexpr uint16_t kResultDisplayMs = 4000U;
constexpr uint16_t kNeutralMin = 250U;
constexpr uint16_t kNeutralMax = 750U;

char g_left_color = 'R';
char g_right_color = 'G';
char g_down_color = 'B';
char g_up_color = 'W';

void gpio_init() {
    DDRB |= (1U << kRedPin) | (1U << kGreenPin) | (1U << kBluePin)
            | (1U << kBuzzerPin) | (1U << kLcdRsPin);
    DDRD |= (1U << kLcdEnablePin) | (1U << kLcdD4Pin) | (1U << kLcdD5Pin)
            | (1U << kLcdD6Pin) | (1U << kLcdD7Pin);

    PORTB &= ~((1U << kRedPin) | (1U << kGreenPin) | (1U << kBluePin)
               | (1U << kBuzzerPin) | (1U << kLcdRsPin));
    PORTD &= ~((1U << kLcdEnablePin) | (1U << kLcdD4Pin) | (1U << kLcdD5Pin)
               | (1U << kLcdD6Pin) | (1U << kLcdD7Pin));
    PORTD |= (1U << kJoystickSwitchPin);
}

void adc_init() {
    ADMUX = (1U << REFS0);
    ADCSRA = (1U << ADEN)
            | (1U << ADPS2)
            | (1U << ADPS1)
            | (1U << ADPS0);
}

uint16_t adc_read_10bit(uint8_t channel) {
    ADMUX = (ADMUX & 0xF0U) | (channel & 0x0FU);

    ADCSRA |= (1U << ADSC);

    while (ADCSRA & (1U << ADSC)) {
    }

    return ADC;
}

void uart_init(uint32_t baud) {
    (void)baud;
    UBRR0 = 103U;
    UCSR0B = (1U << RXEN0) | (1U << TXEN0);
    UCSR0C = (1U << UCSZ01) | (1U << UCSZ00);
}

void uart_putc(char c) {
    while (!(UCSR0A & (1U << UDRE0))) {
    }
    UDR0 = static_cast<uint8_t>(c);
}

void uart_puts(const char* s) {
    while (*s != '\0') {
        uart_putc(*s++);
    }
}

void uart_write_u16(uint16_t value) {
    char buffer[6];
    int8_t index = 5;
    buffer[index] = '\0';

    do {
        buffer[--index] = static_cast<char>('0' + (value % 10U));
        value /= 10U;
    } while (value != 0U);

    uart_puts(&buffer[index]);
}

void uart_write_u32(uint32_t value) {
    char buffer[11];
    int8_t index = 10;
    buffer[index] = '\0';

    do {
        buffer[--index] = static_cast<char>('0' + (value % 10U));
        value /= 10U;
    } while (value != 0U);

    uart_puts(&buffer[index]);
}

void uart_emit_event(const char* status, uint8_t failures, uint16_t duration_ms, bool tamper, uint16_t event_id, uint32_t uptime_ms, const char* user_id) {
    uart_puts("{\"event_id\":\"evt_");
    uart_write_u16(event_id);
    uart_puts("\",\"device_id\":\"CHROMOSTICK_01\",\"user_id\":\"");
    uart_puts(user_id);
    uart_puts("\",\"door\":\"Lab Entrance\"");
    uart_puts(",\"attempt_status\":\"");
    uart_puts(status);
    uart_puts("\",\"consecutive_failures\":");
    uart_write_u16(failures);
    uart_puts(",\"input_duration_ms\":");
    uart_write_u16(duration_ms);
    uart_puts(",\"hardware_tamper_flag\":");
    uart_puts(tamper ? "true" : "false");
    uart_puts(",\"uptime_ms\":");
    uart_write_u32(uptime_ms);
    uart_puts("}\n");
}

void timer_init() {
    TCCR0A = (1U << WGM01);
    TCCR0B = (1U << CS01) | (1U << CS00);
    OCR0A = 249U;
    TIMSK0 = (1U << OCIE0A);
}

ISR(TIMER0_COMPA_vect) {
    ++g_millis;
}

uint32_t custom_millis() {
    return g_millis;
}

void beep(uint16_t frequency, uint16_t duration_ms) {
    tone(11, frequency, duration_ms);
    delay(duration_ms + 50U);
}

void beep_startup() {
    beep(800U, 150U);
    beep(1000U, 150U);
    beep(1200U, 300U);
}

void beep_click() {
    beep(1500U, 60U);
}

void beep_success() {
    beep(1200U, 1000U);
    delay(1500U);
}

void beep_failure() {
    beep(300U, 400U);
    delay(200U);
    beep(300U, 400U);
    delay(600U);
}

void beep_reset() {
    beep(400U, 400U);
}

void set_led_color(char color) {
    PORTB &= ~((1U << kRedPin) | (1U << kGreenPin) | (1U << kBluePin));

    switch (color) {
        case 'R':
            PORTB |= (1U << kRedPin);
            break;
        case 'G':
            PORTB |= (1U << kGreenPin);
            break;
        case 'B':
            PORTB |= (1U << kBluePin);
            break;
        case 'Y':
            PORTB |= (1U << kBluePin);
            break;
        case 'W':
            PORTB |= (1U << kRedPin) | (1U << kGreenPin) | (1U << kBluePin);
            break;
        default:
            break;
    }
}

void lcd_pulse_enable() {
    PORTD |= (1U << kLcdEnablePin);
    _delay_us(1U);
    PORTD &= ~(1U << kLcdEnablePin);
    _delay_us(40U);
}

void lcd_write_nibble(uint8_t nibble) {
    if (nibble & 0x01U) {
        PORTD |= (1U << kLcdD4Pin);
    } else {
        PORTD &= ~(1U << kLcdD4Pin);
    }
    if (nibble & 0x02U) {
        PORTD |= (1U << kLcdD5Pin);
    } else {
        PORTD &= ~(1U << kLcdD5Pin);
    }
    if (nibble & 0x04U) {
        PORTD |= (1U << kLcdD6Pin);
    } else {
        PORTD &= ~(1U << kLcdD6Pin);
    }
    if (nibble & 0x08U) {
        PORTD |= (1U << kLcdD7Pin);
    } else {
        PORTD &= ~(1U << kLcdD7Pin);
    }
    lcd_pulse_enable();
}

void lcd_write_command(uint8_t cmd) {
    PORTB &= ~(1U << kLcdRsPin);
    lcd_write_nibble(cmd >> 4U);
    lcd_write_nibble(cmd & 0x0FU);
    _delay_us(40U);
}

void lcd_write_data(uint8_t data) {
    PORTB |= (1U << kLcdRsPin);
    lcd_write_nibble(data >> 4U);
    lcd_write_nibble(data & 0x0FU);
    _delay_us(40U);
}

void lcd_init() {
    _delay_ms(50U);
    lcd_write_nibble(0x03U);
    _delay_ms(5U);
    lcd_write_nibble(0x03U);
    _delay_us(100U);
    lcd_write_nibble(0x03U);
    lcd_write_nibble(0x02U);
    lcd_write_command(0x28U);
    lcd_write_command(0x0CU);
    lcd_write_command(0x06U);
    lcd_write_command(0x01U);
    _delay_ms(2U);
}

void lcd_clear() {
    lcd_write_command(0x01U);
    _delay_ms(2U);
}

void lcd_set_cursor(uint8_t row, uint8_t col) {
    const uint8_t address = row == 0U ? col : (0x40U + col);
    lcd_write_command(0x80U | address);
}

void lcd_print(const char* text) {
    while (*text != '\0') {
        lcd_write_data(static_cast<uint8_t>(*text));
        ++text;
    }
}

void lcd_print_input(const AuthState& state) {
    lcd_set_cursor(1U, 0U);
    lcd_print("                ");
    lcd_set_cursor(1U, 0U);
    lcd_print(state.input_code);
}

void randomize_direction_mapping() {
    static bool seeded = false;
    if (!seeded) {
        srand(static_cast<unsigned int>(custom_millis()));
        seeded = true;
    }

    switch (rand() % 4U) {
        case 0U:
            g_left_color = 'R';
            g_right_color = 'G';
            g_down_color = 'B';
            g_up_color = 'W';
            break;
        case 1U:
            g_left_color = 'G';
            g_right_color = 'R';
            g_down_color = 'W';
            g_up_color = 'B';
            break;
        case 2U:
            g_left_color = 'B';
            g_right_color = 'W';
            g_down_color = 'R';
            g_up_color = 'G';
            break;
        default:
            g_left_color = 'W';
            g_right_color = 'B';
            g_down_color = 'G';
            g_up_color = 'R';
            break;
    }
}

void show_prompt() {
    lcd_clear();
    lcd_set_cursor(0U, 0U);
    lcd_print("L=");
    lcd_write_data(static_cast<uint8_t>(g_left_color));
    lcd_print(" R=");
    lcd_write_data(static_cast<uint8_t>(g_right_color));
    lcd_set_cursor(1U, 0U);
    lcd_print("D=");
    lcd_write_data(static_cast<uint8_t>(g_down_color));
    lcd_print(" U=");
    lcd_write_data(static_cast<uint8_t>(g_up_color));
}

void show_input_prompt() {
    lcd_clear();
    lcd_set_cursor(0U, 0U);
    lcd_print("Enter PASSCHROMO");
    lcd_set_cursor(1U, 0U);
    lcd_print("Input:");
}

void show_access_granted() {
    lcd_clear();
    lcd_set_cursor(0U, 0U);
    lcd_print("ACCESS GRANTED");
    lcd_set_cursor(1U, 0U);
    lcd_print("WELCOME");
}

void show_access_denied() {
    lcd_clear();
    lcd_set_cursor(0U, 0U);
    lcd_print("Analyzing...");
    lcd_set_cursor(1U, 0U);
    lcd_print("Please wait");
}

void reset_state(AuthState& state) {
    state.input_length = 0U;
    state.input_code[0] = '\0';
    state.neutral = true;
    state.attempt_start_ms = 0U;
    state.tamper_flag = false;
    randomize_direction_mapping();
}

char detect_color(uint16_t xVal, uint16_t yVal) {
    if (xVal < kNeutralMin) {
        return g_left_color;
    }
    if (xVal > kNeutralMax) {
        return g_right_color;
    }
    if (yVal < kNeutralMin) {
        return g_down_color;
    }
    if (yVal > kNeutralMax) {
        return g_up_color;
    }
    return '\0';
}

void record_token(AuthState& state, char token) {
    if (state.input_length < 15U) {
        if (state.attempt_start_ms == 0U) {
            state.attempt_start_ms = custom_millis();
        }
        state.input_code[state.input_length++] = token;
        state.input_code[state.input_length] = '\0';
        set_led_color(token);
        beep_click();
        lcd_print_input(state);
    }
}

const char* match_user(const char* input_code) {
    for (uint8_t i = 0U; i < kUserCount; ++i) {
        if (strcmp(input_code, users[i].passchromo) == 0) {
            return users[i].user_id;
        }
    }
    return nullptr;
}

}  // namespace

int main() {
    gpio_init();
    adc_init();
    uart_init(9600U);
    timer_init();
    sei();

    lcd_init();

    AuthState state;
    reset_state(state);

    uart_puts("ChromoStick edge node ready\n");
    show_prompt();
    beep_startup();
    _delay_ms(3000U);
    show_input_prompt();

    for (;;) {
        const uint16_t xVal = adc_read_10bit(kJoystickXPin);
        const uint16_t yVal = adc_read_10bit(kJoystickYPin);

        uart_puts("X=");
        uart_write_u16(xVal);
        uart_puts("  Y=");
        uart_write_u16(yVal);
        uart_puts("\n");

        _delay_ms(300);

        if ((PIND & (1U << kJoystickSwitchPin)) == 0U) {
            beep_reset();
            reset_state(state);
            show_prompt();
            _delay_ms(kInstructionDisplayMs);

        if (xVal >= kNeutralMin && xVal <= kNeutralMax && yVal >= kNeutralMin && yVal <= kNeutralMax) {
            state.neutral = true;
            _delay_ms(10U);
            continue;
        }

        if (state.neutral) {
            const char token = detect_color(xVal, yVal);
            if (token != '\0') {
                record_token(state, token);
                state.neutral = false;
            }
        }

        if (state.input_length >= kSecretLength) {
            const uint32_t duration_ms = (state.attempt_start_ms != 0U) ? (custom_millis() - state.attempt_start_ms) : 0U;
            const char* matched_user = match_user(state.input_code);
            const bool success = (matched_user != nullptr);
            if (success) {
                state.consecutive_failures = 0U;
                state.tamper_flag = false;
                show_access_granted();
                beep_success();
                uart_emit_event("SUCCESS", state.consecutive_failures, static_cast<uint16_t>(duration_ms), state.tamper_flag, state.event_counter++, custom_millis(), matched_user);
            } else {
                ++state.consecutive_failures;
                state.tamper_flag = (state.consecutive_failures >= 5U);
                show_access_denied();
                beep_failure();
                uart_emit_event("FAILURE", state.consecutive_failures, static_cast<uint16_t>(duration_ms), state.tamper_flag, state.event_counter++, custom_millis(), "UNKNOWN");
            }
            reset_state(state);
            show_prompt();
            _delay_ms(kResultDisplayMs);
        }

        _delay_ms(10U);
    }

    return 0;
}
