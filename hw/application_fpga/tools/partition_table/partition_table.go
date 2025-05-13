package main

import (
	"encoding/binary"
	"flag"
	"fmt"
	"io"
	"os"

	"golang.org/x/crypto/blake2s"
)

// Maximum allowed size in bytes of a preloaded app.
const MaxAppSize = (128 * 1024)

// Size in bytes of the partition table binary. Find out with -o and
// check the file size.
const PartitionSize = 365

type PreLoadedAppData struct {
	Size      uint32
	Digest    [32]uint8
	Signature [64]uint8
}

type Auth struct {
	Nonce      [16]uint8
	AuthDigest [16]uint8
}

type AppStorage struct {
	Status uint8
	Auth   Auth
}

type PartTable struct {
	Version          uint8
	PreLoadedAppData [2]PreLoadedAppData
	AppStorage       [4]AppStorage
}

type PartTableStorage struct {
	PartTable PartTable
	Checksum  [32]byte
}

func (p *PartTableStorage) GenChecksum() {
	buf := make([]byte, 4096)
	len, err := binary.Encode(buf, binary.LittleEndian, p.PartTable)
	if err != nil {
		panic(err)
	}

	p.Checksum = blake2s.Sum256(buf[:len])
}

// Name		Size		Start addr
// ----		----		----
// Bitstream	128KiB		0x00
// ----		----		----
// Partition	64KiB		0x20000
// ----		----		----
// Pre load 0	128KiB		0x30000
// Pre load 1	128KiB		0x50000
// ----		----		----
// storage 0	128KiB		0x70000
// storage 1	128KiB		0x90000
// storage 2	128KiB		0xB0000
// storage 3	128KiB		0xD0000
// ----		----		----
// Partition2   64KiB		0xf0000
type Flash struct {
	Bitstream              [0x20000]uint8                 // Reserved for FPGA bitstream
	PartitionTable         PartTableStorage               // 0x20000 partition table
	PartitionTablePadding  [64*1024 - PartitionSize]uint8 // ~64k padding
	PreLoadedApp0          [0x20000]uint8                 // 0x30000
	PreLoadedApp1          [0x20000]uint8                 // 0x50000
	AppStorage             [4][0x20000]uint8              // 0x70000, 4 * 128 storage for apps
	PartitionTable2        PartTableStorage               // 0xf0000 second copy of table
	PartitionTablePadding2 [64*1024 - PartitionSize]uint8 // ~64k padding
}

func readStruct[T PartTableStorage | Flash](filename string) T {
	var s T

	file, err := os.Open(filename)
	if err != nil {
		panic(err)
	}

	if err := binary.Read(file, binary.LittleEndian, &s); err != nil {
		panic(err)
	}

	sLen, err := file.Seek(0, io.SeekCurrent)
	if err != nil {
		panic(err)
	}

	fmt.Fprintf(os.Stderr, "INFO: %T struct is %d byte long\n", *new(T), sLen)

	return s
}

func printPartTableStorageCondensed(storage PartTableStorage) {
	fmt.Printf("Partition Table Storage\n")
	fmt.Printf("  Partition Table\n")
	fmt.Printf("    Header\n")
	fmt.Printf("      Version          : %d\n", storage.PartTable.Version)

	for i, appData := range storage.PartTable.PreLoadedAppData {
		fmt.Printf("    Preloaded App %d\n", i)
		fmt.Printf("      Size             : %d\n", appData.Size)
		fmt.Printf("      Digest           : %x\n", appData.Digest[:16])
		fmt.Printf("                         %x\n", appData.Digest[16:])
		fmt.Printf("      Signature        : %x\n", appData.Signature[:16])
		fmt.Printf("                         %x\n", appData.Signature[16:32])
		fmt.Printf("                         %x\n", appData.Signature[32:48])
		fmt.Printf("                         %x\n", appData.Signature[48:])
	}
	fmt.Printf("  Digest               : %x\n", storage.Checksum)
}

func genPartitionFile(outputFilename string, app0Filename string) {
	partition := newPartTable(app0Filename)
	partition.GenChecksum()

	storageFile, err := os.Create(outputFilename)
	if err != nil {
		panic(err)
	}

	if err := binary.Write(storageFile, binary.LittleEndian, partition); err != nil {
		panic(err)
	}
}

// newPartTable generates a new partition table suitable for storage.
// If given app0Filename it also fills in the size of the file.
//
// When you're done with filling in the struct, remember to call
// GenChecksum().
//
// It returns the partition table.
func newPartTable(app0Filename string) PartTableStorage {
	storage := PartTableStorage{
		PartTable: PartTable{
			Version:          1,
			PreLoadedAppData: [2]PreLoadedAppData{},
			AppStorage:       [4]AppStorage{},
		},
	}

	if app0Filename != "" {
		stat, err := os.Stat(app0Filename)
		if err != nil {
			panic(err)
		}
		storage.PartTable.PreLoadedAppData[0].Size = uint32(stat.Size())
	}

	return storage
}

func memset(s []byte, c byte) {
	for i := range s {
		s[i] = c
	}
}

func genFlashFile(outputFilename string, app0Filename string) {
	var flash Flash

	// Set all bits in flash to erased.
	memset(flash.Bitstream[:], 0xff)
	// partition0 will be filled in below
	memset(flash.PartitionTablePadding[:], 0xff)
	memset(flash.PreLoadedApp0[:], 0xff)
	memset(flash.PreLoadedApp1[:], 0xff)
	memset(flash.AppStorage[0][:], 0xff)
	memset(flash.AppStorage[1][:], 0xff)
	memset(flash.AppStorage[2][:], 0xff)
	memset(flash.AppStorage[3][:], 0xff)
	// partition1 will be filled in below
	memset(flash.PartitionTablePadding2[:], 0xff)

	partition := newPartTable(app0Filename)
	partition.GenChecksum()

	flash.PartitionTable = partition
	flash.PartitionTable2 = partition

	// Read the entire file.
	appBuf, err := os.ReadFile(app0Filename)
	if err != nil {
		panic(err)
	}

	if len(appBuf) > MaxAppSize {
		fmt.Printf("max app size is 128 k")
		os.Exit(1)
	}

	copy(flash.PreLoadedApp0[:], appBuf)

	storageFile, err := os.Create(outputFilename)
	if err != nil {
		panic(err)
	}

	if err := binary.Write(storageFile, binary.LittleEndian, flash); err != nil {
		panic(err)
	}
}

func main() {
	var input string
	var output string
	var app0 string
	var flash bool

	flag.StringVar(&input, "i", "", "Input binary file. Cannot be used with -o.")
	flag.StringVar(&output, "o", "", "Output binary file. Cannot be used with -i. If used with -f, -app0 must also be specified.")
	flag.StringVar(&app0, "app0", "", "Binary in pre loaded app slot 0. Can be used with -o.")
	flag.BoolVar(&flash, "f", false, "Treat file as a dump of the entire flash memory.")
	flag.Parse()

	if len(flag.Args()) > 0 {
		fmt.Printf("superfluous args\n")
		flag.Usage()
		os.Exit(1)
	}

	if (input == "" && output == "") || (input != "" && output != "") {
		flag.Usage()
		os.Exit(1)
	}

	if input != "" {
		var storage PartTableStorage

		if flash {
			storage = readStruct[Flash](input).PartitionTable
		} else {
			storage = readStruct[PartTableStorage](input)
		}
		printPartTableStorageCondensed(storage)
		os.Exit(0)
	}

	if output != "" {
		if flash {
			if app0 == "" {
				fmt.Printf("need -app0 path/to/app\n")
				os.Exit(1)
			}

			genFlashFile(output, app0)
		} else {
			genPartitionFile(output, app0)
		}
	}
}
