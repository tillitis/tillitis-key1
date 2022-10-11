//======================================================================
//
// touch_sense.v
// -------------
// Touch sensor handler.
//
//
// Author: Joachim Strombergson
// Copyright (C) 2022 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only
//
//======================================================================

`default_nettype none

module touch_sense(
		   input wire           clk,
		   input wire           reset_n,

		   input wire           touch_event,

		   input wire           cs,
		   input wire           we,

		   input wire  [7 : 0]  address,
		   output wire [31 : 0] read_data,
		   output wire          ready
		  );


  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  localparam ADDR_STATUS      = 8'h09;
  localparam STATUS_EVENT_BIT = 0;

  localparam CTRL_IDLE        = 2'h0;
  localparam CTRL_EVENT       = 2'h1;
  localparam CTRL_WAIT        = 2'h2;


  //----------------------------------------------------------------
  // Registers including update variables and write enable.
  //----------------------------------------------------------------
  reg         touch_event_sample0_reg;
  reg         touch_event_sample1_reg;

  reg         touch_event_reg;
  reg         touch_event_new;
  reg         touch_event_set;
  reg         touch_event_rst;
  reg         touch_event_we;

  reg [1 : 0] touch_sense_ctrl_reg;
  reg [1 : 0] touch_sense_ctrl_new;
  reg         touch_sense_ctrl_we;


  //----------------------------------------------------------------
  // Wires.
  //----------------------------------------------------------------
  reg [31 : 0] tmp_read_data;
  reg          tmp_ready;
  reg          api_clear_event;


  //----------------------------------------------------------------
  // Concurrent connectivity for ports etc.
  //----------------------------------------------------------------
  assign read_data = tmp_read_data;
  assign ready     = tmp_ready;


  //----------------------------------------------------------------
  // reg_update
  //----------------------------------------------------------------
  always @ (posedge clk)
    begin : reg_update
      if (!reset_n) begin
	touch_sense_ctrl_reg    <= CTRL_IDLE;
	touch_event_sample0_reg <= 1'h0;
	touch_event_sample1_reg <= 1'h0;
	touch_event_reg         <= 1'h0;
      end

      else begin
	touch_event_sample0_reg <= touch_event;
	touch_event_sample1_reg <= touch_event_sample0_reg;

	if (touch_event_we) begin
	  touch_event_reg <= touch_event_new;
	end

	if (touch_sense_ctrl_we) begin
	  touch_sense_ctrl_reg <= touch_sense_ctrl_new;
	end
      end
    end // reg_update


  //----------------------------------------------------------------
  // api
  //----------------------------------------------------------------
  always @*
    begin : api
      api_clear_event = 1'h0;
      tmp_read_data   = 32'h0;
      tmp_ready       = 1'h0;

      if (cs) begin
	tmp_ready = 1'h1;

        if (we) begin
	  if (address == ADDR_STATUS) begin
	    api_clear_event = 1'h1;
	  end
	end

        else begin
	  if (address == ADDR_STATUS) begin
	    tmp_read_data[STATUS_EVENT_BIT] = touch_event_reg;
	  end
        end
      end
    end // api


  //----------------------------------------------------------------
  // touch_event_reg_logic
  //----------------------------------------------------------------
  always @*
    begin : touch_event_reg_logic
      touch_event_new = 1'h0;
      touch_event_we  = 1'h0;

      if (touch_event_set) begin
	touch_event_new = 1'h1;
	touch_event_we  = 1'h1;
      end

      else if (touch_event_rst) begin
	touch_event_new = 1'h0;
	touch_event_we  = 1'h1;
      end
    end


  //----------------------------------------------------------------
  // touch_sense_ctrl
  //----------------------------------------------------------------
  always @*
    begin : touch_sense_ctrl
      touch_event_set      = 1'h0;
      touch_event_rst      = 1'h0;
      touch_sense_ctrl_new = CTRL_IDLE;
      touch_sense_ctrl_we  = 1'h0;

      case (touch_sense_ctrl_reg)
	CTRL_IDLE : begin
	  if (touch_event_sample1_reg) begin
	    touch_event_set      = 1'h1;
	    touch_sense_ctrl_new = CTRL_EVENT;
	    touch_sense_ctrl_we  = 1'h1;
	  end
	end

	CTRL_EVENT: begin
	  if (api_clear_event) begin
	    touch_event_rst      = 1'h1;
	    touch_sense_ctrl_new = CTRL_WAIT;
	    touch_sense_ctrl_we  = 1'h1;
	  end
	end

	CTRL_WAIT: begin
	  if (!touch_event_sample1_reg) begin
	    touch_sense_ctrl_new = CTRL_IDLE;
	    touch_sense_ctrl_we  = 1'h1;
	  end
	end

	default : begin
	end
      endcase // case (touch_sense_ctrl_reg)
    end

endmodule // touch_sense

//======================================================================
// EOF touch_sense.v
//======================================================================
