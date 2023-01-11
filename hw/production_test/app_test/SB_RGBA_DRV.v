

module SB_RGBA_DRV #(
    parameter CURRENT_MODE,
    parameter RGB0_CURRENT,
    parameter RGB1_CURRENT,
    parameter RGB2_CURRENT
) (
    output reg RGB0,
    output reg RGB1,
    output reg RGB2,

    input RGBLEDEN,
    input RGB0PWM,
    input RGB1PWM,
    input RGB2PWM,
    input CURREN
);

    // Nonfunctional, for linting only.
    always @(*) begin
        RGB0 = (RGBLEDEN & CURREN & RGB0PWM);
    end

    always @(*) begin
        RGB1 = (RGBLEDEN & CURREN & RGB1PWM);
    end

    always @(*) begin
        RGB2 = (RGBLEDEN & CURREN & RGB2PWM);
    end

endmodule

