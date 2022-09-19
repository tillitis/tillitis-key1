module top (

    input  HOST_SS,
    output reg HOST_SCK,
    input  HOST_MOSI,
    output reg HOST_MISO,
    input  HOST_EXTRA1,
    output reg HOST_EXTRA2,
    input  HOST_EXTRA3,
    output reg HOST_EXTRA4,
    input  HOST_EXTRA5,
    output reg HOST_EXTRA6,
    input  HOST_EXTRA7,
    output reg HOST_EXTRA8,
    input  HOST_EXTRA9,
    output reg HOST_EXTRA10,


    output APP_GPIO1,
    output APP_GPIO2,
    output APP_GPIO3,
    output APP_GPIO4,

    input APP_BUTTON,

    output RGB0,
    output RGB1,
    output RGB2
);

    //############ Feedback test ############################################

    always @(posedge HOST_SS) begin
        HOST_SCK <= ~HOST_SCK;
    end

    always @(posedge HOST_MOSI) begin
        HOST_MISO <= ~HOST_MISO;
    end

    always @(posedge HOST_EXTRA1) begin
        HOST_EXTRA2 <= ~HOST_EXTRA2;
    end

    always @(posedge HOST_EXTRA3) begin
        HOST_EXTRA4 <= ~HOST_EXTRA4;
    end

    always @(posedge HOST_EXTRA5) begin
        HOST_EXTRA6 <= ~HOST_EXTRA6;
    end

    always @(posedge HOST_EXTRA7) begin
        HOST_EXTRA8 <= ~HOST_EXTRA8;
    end

    always @(posedge HOST_EXTRA9) begin
        HOST_EXTRA10 <= ~HOST_EXTRA10;
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
        //.RGBLEDEN(1'b1),
        .RGBLEDEN(APP_BUTTON),
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

    assign APP_GPIO1 = clk;
    assign APP_GPIO2 = HOST_SS;
    assign APP_GPIO3 = HOST_EXTRA8;
    assign APP_GPIO4 = HOST_EXTRA10;


endmodule
