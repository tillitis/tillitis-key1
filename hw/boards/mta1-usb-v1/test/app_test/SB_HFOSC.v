
module SB_HFOSC #(
    parameter CLKHF_DIV
) (
    input CLKHFPU,
    input CLKHFEN,

    output reg CLKHF
);

    // Nonfunctional, for linting only.
    always @(*) begin
        CLKHF = (CLKHFPU & CLKHFEN);
    end

endmodule

