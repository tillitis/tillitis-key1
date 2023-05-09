
module SB_RGBA_DRV (
                 output wire RGB0,
                 output wire RGB1,
                 output wire RGB2,
                 input wire RGBLEDEN,
                 input wire RGB0PWM,
                 input wire RGB1PWM,
                 input wire RGB2PWM,
                 input wire CURREN
		);

  parameter CURRENT_MODE = 1;
  parameter RGB0_CURRENT = 8'h0;
  parameter RGB1_CURRENT = 8'h0;
  parameter RGB2_CURRENT = 8'h0;

  assign RGB0 = RGB0PWM;
  assign RGB1 = RGB1PWM;
  assign RGB2 = RGB2PWM;

endmodule // SB_RGBA_DRV
