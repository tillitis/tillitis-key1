package main

import (
	"encoding/binary"
	"flag"
	"fmt"
	"io"
	"os"

	"golang.org/x/crypto/blake2s"
)

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
	Digest    [16]uint8
}

type Flash struct {
	Bitstream             [0x20000]uint8
	PartitionTableStorage PartTableStorage
	PartitionTablePadding [64*1024 - 349]uint8
	PreLoadedApp0         [0x20000]uint8
	PreLoadedApp1         [0x20000]uint8
	AppStorage            [4][0x20000]uint8
	EndPadding            [0x10000]uint8
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
	fmt.Printf("  Digest               : %x\n", storage.Digest)
}

func calculateStorageDigest(data []byte) []byte {
	key := [16]byte{}

	hash, err := blake2s.New128(key[:])
	if err != nil {
		panic(err)
	}

	hash.Write(data)
	digest := hash.Sum([]byte{})

	return digest
}

func generatePartTableStorage(outputFilename string, app0Filename string) {
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

	buf := make([]byte, 4096, 4096)
	len, err := binary.Encode(buf, binary.LittleEndian, storage.PartTable)
	if err != nil {
		panic(err)
	}

	digest := calculateStorageDigest(buf[:len])
	copy(storage.Digest[:], digest)

	storageFile, err := os.Create(outputFilename)
	if err != nil {
		panic(err)
	}

	if err := binary.Write(storageFile, binary.LittleEndian, storage); err != nil {
		panic(err)
	}
}

func main() {
	input := ""
	output := ""
	app0 := ""
	flash := false

	flag.StringVar(&input, "i", "", "Input binary dump file. Cannot be used with -o.")
	flag.StringVar(&output, "o", "", "Output binary dump file. Cannot be used with -i.")
	flag.StringVar(&app0, "app0", "", "Binary in pre loaded app slot 0. Can be used with -o.")
	flag.BoolVar(&flash, "f", false, "Treat input file as a dump of the entire flash memory.")
	flag.Parse()

	if (input == "" && output == "") || (input != "" && output != "") {
		flag.Usage()
		os.Exit(1)
	}

	if input != "" {
		var storage PartTableStorage

		if flash {
			storage = readStruct[Flash](input).PartitionTableStorage
		} else {
			storage = readStruct[PartTableStorage](input)
		}
		printPartTableStorageCondensed(storage)
	} else if output != "" {
		generatePartTableStorage(output, app0)
	}
}
