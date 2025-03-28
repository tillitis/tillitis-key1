package main

import (
	"encoding/binary"
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"os"
)

type PartTable struct {
	Header struct {
		Version uint8
	}
	PreLoadedAppData [2]struct {
		Size      uint32
		Digest    [32]uint8
		Signature [64]uint8
	}
	AppStorage [4]struct {
		Status uint8
		Auth   struct {
			Nonce      [16]uint8
			AuthDigest [16]uint8
		}
	}
}

type Flash struct {
	Bitstream             [0x20000]uint8
	PartitionTable        PartTable
	PartitionTablePadding [64*1024 - 365]uint8
	PreLoadedApp0         [0x20000]uint8
	PreLoadedApp1         [0x20000]uint8
	AppStorage            [4][0x20000]uint8
	EndPadding            [0x10000]uint8
}

func readPartTable(filename string) PartTable {
	var tbl PartTable

	tblFile, err := os.Open(filename)
	if err != nil {
		panic(err)
	}

	if err := binary.Read(tblFile, binary.LittleEndian, &tbl); err != nil {
		fmt.Println("binary.Read failed:", err)
	}

	tblStructLen, _ := tblFile.Seek(0, io.SeekCurrent)
	fmt.Fprintf(os.Stderr, "INFO: Go partition table struct is %d byte long\n", tblStructLen)

	return tbl
}

func readFlashDump(filename string) Flash {
	var flash Flash

	flashFile, err := os.Open(filename)
	if err != nil {
		panic(err)
	}

	if err := binary.Read(flashFile, binary.LittleEndian, &flash); err != nil {
		fmt.Println("binary.Read failed:", err)
	}

	flashStructLen, _ := flashFile.Seek(0, io.SeekCurrent)
	fmt.Fprintf(os.Stderr, "INFO: Go flash table struct is %d Mbyte long\n", flashStructLen/1024/1024)

	return flash
}

func printPartTableJson(tbl PartTable) {
	partTableJSON, err := json.MarshalIndent(tbl, "", "  ")
	if err != nil {
		panic(err)
	}
	fmt.Printf("%s\n", string(partTableJSON))
}

func printPartTableCondensed(tbl PartTable) {
	fmt.Printf("Header\n")
	fmt.Printf("  Version          : %d\n", tbl.Header.Version)

	for i, appData := range tbl.PreLoadedAppData {
		fmt.Printf("Preloaded App %d\n", i)
		fmt.Printf("  Size             : %d\n", appData.Size)
		fmt.Printf("  Digest           : %x\n", appData.Digest[:16])
		fmt.Printf("                     %x\n", appData.Digest[16:])
		fmt.Printf("  Signature        : %x\n", appData.Signature[:16])
		fmt.Printf("                     %x\n", appData.Signature[16:32])
		fmt.Printf("                     %x\n", appData.Signature[32:48])
		fmt.Printf("                     %x\n", appData.Signature[48:])
	}
}

func main() {
	filename := ""
	json := false
	flash := false

	flag.StringVar(&filename, "i", "", "Partition table binary dump file.")
	flag.BoolVar(&json, "j", false, "Print partition table in JSON format.")
	flag.BoolVar(&flash, "f", false, "Treat input file as a dump of the entire flash memory.")
	flag.Parse()

	if filename == "" {
		flag.Usage()
		os.Exit(1)
	}

	var tbl PartTable

	if flash {
		tbl = readFlashDump(filename).PartitionTable
	} else {
		tbl = readPartTable(filename)
	}

	if json {
		printPartTableJson(tbl)
	} else {
		printPartTableCondensed(tbl)
	}
}
