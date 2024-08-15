//======================================================================
//
// timer_core.v
// ------------
// timer core.
//
//
// Author: Joachim Strombergson
// Copyright (C) 2022 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only
//
//======================================================================

`default_nettype none

module timer_core(
                  input wire           clk,
                  input wire           reset_n,

                  input wire [31 : 0]  prescaler_init,
                  input wire [31 : 0]  timer_init,
                  input wire           start,
                  input wire           stop,
                  input wire           free_running,

                  output wire [31 : 0] curr_timer,
                  output wire          running
                  );


  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  localparam CTRL_IDLE      = 2'h0;
  localparam CTRL_PRESCALER = 2'h1;
  localparam CTRL_TIMER     = 2'h2;


  //----------------------------------------------------------------
  // Registers including update variables and write enable.
  //----------------------------------------------------------------
  reg          running_reg;
  reg          running_new;
  reg          running_we;

  reg [31 : 0] prescaler_reg;
  reg [31 : 0] prescaler_new;
  reg          prescaler_we;
  reg          prescaler_rst;
  reg          prescaler_inc;

  reg [31 : 0] timer_reg;
  reg [31 : 0] timer_new;
  reg          timer_we;
  reg          timer_rst;
  reg          timer_inc;

  reg [1 : 0]  core_ctrl_reg;
  reg [1 : 0]  core_ctrl_new;
  reg          core_ctrl_we;


  //----------------------------------------------------------------
  // Concurrent connectivity for ports etc.
  //----------------------------------------------------------------
  assign curr_timer = timer_reg;
  assign running    = running_reg;


  //----------------------------------------------------------------
  // reg_update
  //----------------------------------------------------------------
  always @ (posedge clk)
    begin: reg_update
      if (!reset_n)
        begin
          running_reg   <= 1'h0;
	  prescaler_reg <= 32'h0;
	  timer_reg     <= 32'h0;
          core_ctrl_reg <= CTRL_IDLE;
        end
      else
        begin
          if (running_we) begin
            running_reg <= running_new;
	  end

	  if (prescaler_we) begin
	    prescaler_reg <= prescaler_new;
	  end

	  if (timer_we) begin
	    timer_reg <= timer_new;
	  end

          if (core_ctrl_we) begin
            core_ctrl_reg <= core_ctrl_new;
          end
	end
    end // reg_update


  //----------------------------------------------------------------
  // prescaler_ctr
  //----------------------------------------------------------------
  always @*
    begin : prescaler_ctr
      prescaler_new = 32'h0;
      prescaler_we  = 1'h0;

      if (prescaler_rst) begin
	prescaler_new = 32'h0;
	prescaler_we  = 1'h1;
      end
      else if (prescaler_inc) begin
	prescaler_new = prescaler_reg + 1'h1;
	prescaler_we  = 1'h1;
      end
    end


  //----------------------------------------------------------------
  // timer_ctr
  //----------------------------------------------------------------
  always @*
    begin : timer_ctr
      timer_new = 32'h0;
      timer_we  = 1'h0;

      if (timer_rst) begin
	timer_new = 32'h0;
	timer_we  = 1'h1;
      end
      else if (timer_inc) begin
	timer_new = timer_reg + 1'h1;
	timer_we  = 1'h1;
      end
    end


  //----------------------------------------------------------------
  // Core control FSM.
  //----------------------------------------------------------------
  always @*
    begin : core_ctrl
      running_new   = 1'h0;
      running_we    = 1'h0;
      prescaler_rst = 1'h0;
      prescaler_inc = 1'h0;
      timer_rst     = 1'h0;
      timer_inc     = 1'h0;
      core_ctrl_new = CTRL_IDLE;
      core_ctrl_we  = 1'h0;

      case (core_ctrl_reg)
        CTRL_IDLE: begin
          if (start) begin
            running_new   = 1'h1;
            running_we    = 1'h1;
	    prescaler_rst = 1'h1;
	    timer_rst     = 1'h1;

	    core_ctrl_new = CTRL_PRESCALER;
	    core_ctrl_we  = 1'h1;
          end
        end


	CTRL_PRESCALER: begin
	  if (stop) begin
            running_new   = 1'h0;
            running_we    = 1'h1;
            core_ctrl_new = CTRL_IDLE;
            core_ctrl_we  = 1'h1;
	  end

	  else begin
	    if (prescaler_reg == (prescaler_init - 1)) begin
	      if ((timer_reg == (timer_init - 1)) & ~free_running) begin
		running_new   = 1'h0;
		running_we    = 1'h1;
		core_ctrl_new = CTRL_IDLE;
		core_ctrl_we  = 1'h1;
	      end
	      else begin
		timer_inc     = 1'h1;
		prescaler_rst = 1'h1;
	      end
	    end
	    else begin
	      prescaler_inc = 1'h1;
	    end
	  end
	end

        default: begin
        end
      endcase // case (core_ctrl_reg)
    end // core_ctrl

endmodule // timer_core

//======================================================================
// EOF timer_core.v
//======================================================================
