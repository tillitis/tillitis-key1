// SPDX-FileCopyrightText: 2025 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

package main

import (
	"flag"
	"fmt"
	"os"
	"path"

	"golang.org/x/crypto/blake2s"
)

func usage() {
	fmt.Printf("Usage: %s -m filename [-c]\n", os.Args[0])
}

func printCDigest(digest [blake2s.Size]byte, fileName string) {
	fmt.Printf("#include <stdint.h>\n\n")
	fmt.Printf("// BLAKE2s digest of %v\n", path.Base(fileName))
	fmt.Printf("const uint8_t allowed_app_digest[32] = {\n")

	for _, n := range digest {
		fmt.Printf("0x%02x, ", n)
	}

	fmt.Printf("\n}; \n")
}

func main() {
	var messageFile string
	var forC bool

	flag.StringVar(&messageFile, "m", "", "Specify file containing message.")
	flag.BoolVar(&forC, "c", false, "Print digest for inclusion in C program.")

	flag.Usage = usage
	flag.Parse()

	if messageFile == "" {
		usage()
		os.Exit(0)
	}

	message, err := os.ReadFile(messageFile)
	if err != nil {
		fmt.Printf("%v\n", err)
		os.Exit(1)
	}

	digest := blake2s.Sum256(message)

	if forC {
		printCDigest(digest, messageFile)
	} else {
		fmt.Printf("%x %s\n", digest, messageFile)
	}

	os.Exit(0)
}
