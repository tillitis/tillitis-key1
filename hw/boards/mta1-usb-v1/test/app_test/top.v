module top (
    /* verilator lint_off UNUSED */
    input RX,
    /* verilator lint_on UNUSED */
    output reg TX,
    input RTS,
    output CTS,

    output APP_GPIO1,
    output APP_GPIO2,
    output APP_GPIO3,
    output APP_GPIO4,

    input TOUCH_EVENT,

    output RGB0,
    output RGB1,
    output RGB2
);


    //############ Clock / Reset ############################################

    wire clk;

    // Configure the HFOSC
	SB_HFOSC #(
        .CLKHF_DIV("0b01") // 00: 48MHz, 01: 24MHz, 10: 12MHz, 11: 6MHz
    ) u_hfosc (
       	.CLKHFPU(1'b1),
       	.CLKHFEN(1'b1),
        .CLKHF(clk)
    );

    //############ Extra I/O test #####################################

    reg [4:0] pintest;
    initial begin
        pintest = 5'h01;
    end

    assign {CTS, APP_GPIO1, APP_GPIO2, APP_GPIO3, APP_GPIO4} = pintest;

    always @(posedge RTS) begin
        case(pintest)
            5'h1: pintest <= 5'h2; 
            5'h2: pintest <= 5'h4;
            5'h4: pintest <= 5'h8;
            5'h8: pintest <= 5'h10;
            default: pintest <= 5'h01;
    endcase
    end

    //############ LED pwm ##################################################

    reg [24:0] led_divider;
    reg [2:0] led_states;

    initial begin
	led_divider = 25'd0;
    end

    always @(posedge clk) begin
        led_divider <= led_divider + 1;

	case(led_divider[24:23])
	    2'h0: led_states <= 3'b100;
	    2'h1: led_states <= 3'b010;
	    2'h2: led_states <= 3'b001;
	    2'h3: led_states <= 3'b111;
	endcase
    end

    SB_RGBA_DRV #(
        .CURRENT_MODE("0b1"),       // half-current mode
        .RGB0_CURRENT("0b000001"),  // 2 mA
        .RGB1_CURRENT("0b000001"),  // 2 mA
        .RGB2_CURRENT("0b000001")   // 2 mA
    ) RGBA_DRV (
        .RGB0(RGB0),
        .RGB1(RGB1),
        .RGB2(RGB2),
        .RGBLEDEN(~TOUCH_EVENT),
        .RGB0PWM(led_states[2]),
        .RGB1PWM(led_states[1]),
        .RGB2PWM(led_states[0]),
        .CURREN(1'b1)
    );

    //############ Serial output ############################################

    reg [24:0] ticks;   // about once a second
    reg [7:0] touch_count;
    reg [7:0] report_count;
    initial begin
        ticks = 25'd0;
        touch_count = 8'd0;
        report_count = 8'd0;
    end

    reg [19:0] uart_data;
    reg [7:0] uart_bits_left;
    reg [13:0] uart_count;

    initial begin
        TX = 1'b1;
    end

    reg touch_last;

    reg tx_last;
    initial begin
        tx_last = 1;
    end

    always @(posedge clk) begin
        // Sample + count touch events
        touch_last <= TOUCH_EVENT;
        tx_last <= TX;

        if ((touch_last == 0) && (TOUCH_EVENT == 1)) begin
            touch_count <= touch_count + 1;
        end

        // Create a ~1s delay between touch count reports
        ticks <= ticks + 1;

        TX <= uart_data[0];

        // Periodically report the touch count
        //if (ticks == 0) begin
        if((TX == 0) && (tx_last == 1) && (uart_bits_left == 0)) begin
            // LSB first: start bit, report count, stop bit, start bit, touch count, stop bit
            uart_data <= {1'b1, touch_count[7:0], 1'b0, 1'b1, report_count[7:0], 1'b0};
            uart_bits_left <= 8'd19;
            uart_count <= 14'd2500;

            report_count <= report_count + 1;
        end

        if (uart_bits_left > 0) begin

            uart_count <= uart_count - 1;

            if(uart_count == 0) begin
                uart_data <= {1'b1, uart_data[19:1]};

                uart_bits_left <= uart_bits_left - 1;
                uart_count <= 14'd2500;
            end
        end
    end

endmodule
