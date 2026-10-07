// SPDX-FileCopyrightText: 2026 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

package main

import (
	"bytes"
	"encoding/hex"
	"testing"
)

const expectedMACHex = "41f989d1b1afc24bcd39cb373fddc49bb28cf57b0bee1887148b35e6ff18d9f3"

func TestPartTableGenMac(t *testing.T) {
	app0 := bytes.Repeat([]byte{0x00}, 368)

	partition := newPartTable(app0, nil, nil, nil)
	// using the development UDS
	partition.GenMac("")

	got := hex.EncodeToString(partition.Mac[:])

	if got != expectedMACHex {
		t.Errorf("MAC mismatch\nExpected: %s\nGot:      %s",
			expectedMACHex, got)
	}
}

func TestPartTableGenMacChangesWhenTableChanges(t *testing.T) {
	app0 := make([]byte, 368)
	for i := range app0 {
		app0[i] = byte(i)
	}

	partition := newPartTable(app0, nil, nil, nil)

	// using the development UDS
	partition.GenMac("")
	mac1 := partition.Mac

	partition.PartTable.Version++
	// using the development UDS
	partition.GenMac("")
	mac2 := partition.Mac

	if mac1 == mac2 {
		t.Fatal("MAC did not change when partition table changed")
	}
}
