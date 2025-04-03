package main

import (
	"encoding/binary"
	"encoding/json"
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
	PartitionTablePadding [64*1024 - 365]uint8
	PreLoadedApp0         [0x20000]uint8
	PreLoadedApp1         [0x20000]uint8
	AppStorage            [4][0x20000]uint8
	EndPadding            [0x10000]uint8
}

func readPartTableStorage(filename string) PartTableStorage {
	var storage PartTableStorage

	tblFile, err := os.Open(filename)
	if err != nil {
		panic(err)
	}

	if err := binary.Read(tblFile, binary.LittleEndian, &storage); err != nil {
		fmt.Println("binary.Read failed:", err)
	}

	tblStructLen, _ := tblFile.Seek(0, io.SeekCurrent)
	fmt.Fprintf(os.Stderr, "INFO: Go partition table struct is %d byte long\n", tblStructLen)

	return storage
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

func printPartTableStorageJson(storage PartTableStorage) {
	json, err := json.MarshalIndent(storage, "", "  ")
	if err != nil {
		panic(err)
	}
	fmt.Printf("%s\n", string(json))
}

func printPartTableJson(tbl PartTable) {
	partTableJSON, err := json.MarshalIndent(tbl, "", "  ")
	if err != nil {
		panic(err)
	}
	fmt.Printf("%s\n", string(partTableJSON))
}

func printPartTableCondensed(storage PartTableStorage) {
	fmt.Printf("Header\n")
	fmt.Printf("  Version          : %d\n", storage.PartTable.Version)

	for i, appData := range storage.PartTable.PreLoadedAppData {
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

func printPartTableStorageCondensed(storage PartTableStorage) {
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

func generatePartTableStorage(filename string) {
	storage := PartTableStorage{
		PartTable: PartTable{
			Version:          1,
			PreLoadedAppData: [2]PreLoadedAppData{},
			AppStorage:       [4]AppStorage{},
		},
	}

	buf := make([]byte, 4096, 4096)
	len, err := binary.Encode(buf, binary.LittleEndian, storage.PartTable)
	if err != nil {
		panic(err)
	}

	digest := calculateStorageDigest(buf[:len])
	copy(storage.Digest[:], digest)

	storageFile, err := os.Create(filename)
	if err != nil {
		panic(err)
	}

	if err := binary.Write(storageFile, binary.LittleEndian, storage); err != nil {
		fmt.Println("binary.Write failed:", err)
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

	generatePartTableStorage(filename)
	return

	// var tbl PartTable

	// if flash {
	// 	tbl = readFlashDump(filename).PartitionTable
	// } else {
	// 	tbl = readPartTable(filename)
	// }

	// if json {
	// 	printPartTableJson(tbl)
	// } else {
	// 	printPartTableCondensed(tbl)
	// }
}
