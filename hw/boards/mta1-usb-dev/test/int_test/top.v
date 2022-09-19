module top (
    input INT_CLOCK,

    output reg HOST_SS,
    input  HOST_SCK,
    output reg HOST_MOSI,
    input  HOST_MISO,
    output reg HOST_EXTRA1,
    input  HOST_EXTRA2,
    output reg HOST_EXTRA3,
    input  HOST_EXTRA4,
    output reg HOST_EXTRA5,
    input  HOST_EXTRA6,
    output reg HOST_EXTRA7,
    input  HOST_EXTRA8,
    output reg HOST_EXTRA9,
    input  HOST_EXTRA10,


    output INT_GPIO1,
    output INT_GPIO2,
    output INT_GPIO3,
    output INT_GPIO4,

    output RGB0,
    output RGB1,
    output RGB2
);
    //############ Feedback test ############################################

    always @(posedge INT_CLOCK) begin
        HOST_SS <= ~HOST_SS;
    end

    always @(posedge HOST_SCK) begin
        HOST_MOSI <= ~HOST_MOSI;
    end

    always @(posedge HOST_MISO) begin
        HOST_EXTRA1 <= ~HOST_EXTRA1;
    end

    always @(posedge HOST_EXTRA2) begin
        HOST_EXTRA3 <= ~HOST_EXTRA3;
    end

    always @(posedge HOST_EXTRA4) begin
        HOST_EXTRA5 <= ~HOST_EXTRA5;
    end

    always @(posedge HOST_EXTRA6) begin
        HOST_EXTRA7 <= ~HOST_EXTRA7;
    end

    always @(posedge HOST_EXTRA8) begin
        HOST_EXTRA9 <= ~HOST_EXTRA9;
    end

    reg [10:0] slow_led;

    always @(posedge HOST_EXTRA10) begin
        slow_led <= slow_led + 1;
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
        .RGBLEDEN(1'b1),
        .RGB0PWM(slow_led[10]),
        .RGB1PWM(slow_led[9]),
        .RGB2PWM(slow_led[8]),
        .CURREN(1'b1)
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

    //############ GPIO tests ###############################################

    assign INT_GPIO1 = clk;
    assign INT_GPIO2 = INT_CLOCK;
    assign INT_GPIO3 = HOST_EXTRA8;
    assign INT_GPIO4 = HOST_EXTRA10;

endmodule
