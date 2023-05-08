//======================================================================
//
// tb_touch_sense.v
// ----------------
// Testbench for the touch sense core.
//
//
// Author: Joachim Strombergson
// Copyright (C) 2022 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only
//
//======================================================================

`default_nettype none

module tb_touch_sense();

  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  parameter DEBUG     = 1;
  parameter DUMP_WAIT = 0;

  parameter CLK_HALF_PERIOD = 1;
  parameter CLK_PERIOD = 2 * CLK_HALF_PERIOD;

  localparam ADDR_STATUS       = 8'h09;
  localparam STATUS_READY_BIT  = 0;


  //----------------------------------------------------------------
  // Register and Wire declarations.
  //----------------------------------------------------------------
  reg [31 : 0] cycle_ctr;
  reg [31 : 0] error_ctr;
  reg [31 : 0] tc_ctr;
  reg          tb_monitor;

  reg           tb_clk;
  reg           tb_reset_n;
  reg           tb_touch_event;
  reg           tb_cs;
  reg           tb_we;
  reg [7 : 0]   tb_address;
  wire [31 : 0] tb_read_data;

  reg [31 : 0] read_data;


  //----------------------------------------------------------------
  // Device Under Test.
  //----------------------------------------------------------------
  touch_sense dut(
		  .clk(tb_clk),
		  .reset_n(tb_reset_n),

		  .touch_event(tb_touch_event),

		  .cs(tb_cs),
		  .we(tb_we),
		  .address(tb_address),
		  .read_data(tb_read_data)
		 );


  //----------------------------------------------------------------
  // clk_gen
  //
  // Always running clock generator process.
  //----------------------------------------------------------------
  always
    begin : clk_gen
      #CLK_HALF_PERIOD;
      tb_clk = !tb_clk;
    end // clk_gen


  //----------------------------------------------------------------
  // sys_monitor()
  //
  // An always running process that creates a cycle counter and
  // conditionally displays information about the DUT.
  //----------------------------------------------------------------
  always
    begin : sys_monitor
      cycle_ctr = cycle_ctr + 1;
      #(CLK_PERIOD);
      if (tb_monitor)
        begin
          dump_dut_state();
        end
    end


  //----------------------------------------------------------------
  // dump_dut_state()
  //
  // Dump the state of the dump when needed.
  //----------------------------------------------------------------
  task dump_dut_state;
    begin
      $display("State of DUT");
      $display("------------");
      $display("Cycle: %08d", cycle_ctr);
      $display("");
    end
  endtask // dump_dut_state


  //----------------------------------------------------------------
  // reset_dut()
  //
  // Toggle reset to put the DUT into a well known state.
  //----------------------------------------------------------------
  task reset_dut;
    begin
      $display("--- Toggle reset.");
      tb_reset_n = 0;
      #(2 * CLK_PERIOD);
      tb_reset_n = 1;
    end
  endtask // reset_dut


  //----------------------------------------------------------------
  // display_test_result()
  //
  // Display the accumulated test results.
  //----------------------------------------------------------------
  task display_test_result;
    begin
      if (error_ctr == 0)
        begin
          $display("--- All %02d test cases completed successfully", tc_ctr);
        end
      else
        begin
          $display("--- %02d tests completed - %02d test cases did not complete successfully.",
                   tc_ctr, error_ctr);
        end
    end
  endtask // display_test_result


  //----------------------------------------------------------------
  // init_sim()
  //
  // Initialize all counters and testbed functionality as well
  // as setting the DUT inputs to defined values.
  //----------------------------------------------------------------
  task init_sim;
    begin
      cycle_ctr  = 0;
      error_ctr  = 0;
      tc_ctr     = 0;
      tb_monitor = 0;

      tb_clk         = 1'h0;
      tb_reset_n     = 1'h1;
      tb_touch_event = 1'h0;
      tb_cs          = 1'h0;
      tb_we          = 1'h0;
      tb_address     = 8'h0;
    end
  endtask // init_sim


  //----------------------------------------------------------------
  // write_word()
  //
  // Write the given word to the DUT using the DUT interface.
  //----------------------------------------------------------------
  task write_word(input [7 : 0] address,
                  input [31 : 0] word);
    begin
      if (DEBUG)
        begin
          $display("--- Writing 0x%08x to 0x%02x.", word, address);
          $display("");
        end

      tb_address = address;
      tb_cs = 1;
      tb_we = 1;
      #(2 * CLK_PERIOD);
      tb_cs = 0;
      tb_we = 0;
    end
  endtask // write_word


  //----------------------------------------------------------------
  // read_word()
  //
  // Read a data word from the given address in the DUT.
  // the word read will be available in the global variable
  // read_data.
  //----------------------------------------------------------------
  task read_word(input [7 : 0]  address);
    begin
      tb_address = address;
      tb_cs = 1;
      tb_we = 0;
      #(CLK_PERIOD);
      read_data = tb_read_data;
      tb_cs = 0;

      if (DEBUG)
        begin
          $display("--- Reading 0x%08x from 0x%02x.", read_data, address);
          $display("");
        end
    end
  endtask // read_word


  //----------------------------------------------------------------
  // wait_ready()
  //
  // Wait for the ready flag to be set in dut.
  //----------------------------------------------------------------
  task wait_ready;
    begin : wready
      read_word(ADDR_STATUS);
      while (read_data == 0)
        read_word(ADDR_STATUS);
    end
  endtask // wait_ready


  //----------------------------------------------------------------
  // test1.
  // Create a touch event check that it is accessible from the
  // API. Clear the event from the API and check that it is
  // really cleared.
  //----------------------------------------------------------------
  task test1;
    begin
      tc_ctr = tc_ctr + 1;

      $display("");
      $display("--- test1: started.");

      // Set touch event to low:
      tb_touch_event = 1'h0;

      // Clear the event handler.
      $display("--- test1: Clearing any stray event.");
      write_word(8'h09, 32'h0);


      // Check status.
      #(CLK_PERIOD);
      read_word(8'h09);

      // Set touch event input to high.
      $display("--- test1: Creating a touch event.");
      tb_touch_event = 1'h1;

      $display("--- test1: Waiting for the event to be caught.");
      wait_ready();

      $display("--- test1: Event has been seen.");

      $display("--- test1: Dropping the event input.");
      tb_touch_event = 1'h0;
      #(CLK_PERIOD);
      $display("--- test1: Clearing the event.");
      write_word(8'h09, 32'h0);
      #(CLK_PERIOD);

      // Check that the event is now removed.
      read_word(8'h09);
      #(CLK_PERIOD);
      $display("--- test1: Event has been cleared.");
      read_word(8'h09);
      #(CLK_PERIOD);

      $display("--- test1: completed.");
      $display("");
    end
  endtask // test1


  //----------------------------------------------------------------
  // touch_sense_test
  //----------------------------------------------------------------
  initial
    begin : timer_test
      $display("");
      $display("   -= Testbench for touch_sense started =-");
      $display("     ====================================");
      $display("");

      init_sim();
      reset_dut();

      test1();

      display_test_result();
      $display("");
      $display("   -= Testbench for touch_sense completed =-");
      $display("     ======================================");
      $display("");
      $finish;
    end // touch_sense_test
endmodule // tb_touch_sense

//======================================================================
// EOF tb_touch_sense.v
//======================================================================
