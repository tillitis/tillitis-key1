// SPDX-FileCopyrightText: 2023 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

package main

import (
	"bytes"
	"encoding/base64"
	"encoding/binary"
	"fmt"
	"os"
	"strings"
)

type Signature struct {
	Alg    [2]byte
	KeyNum [8]byte
	Sig    [64]byte
}

// ReadBase64 reads the file in filename with base64, decodes it and
// returns a binary representation
func ReadBase64(filename string) ([]byte, error) {
	input, err := os.ReadFile(filename)
	if err != nil {
		return nil, fmt.Errorf("%w", err)
	}

	lines := strings.Split(string(input), "\n")
	if len(lines) < 2 {
		return nil, fmt.Errorf("too few lines in file %s", filename)
	}

	data, err := base64.StdEncoding.DecodeString(lines[1])
	if err != nil {
		return nil, fmt.Errorf("could not decode: %w", err)
	}

	return data, nil
}

func ReadSig(filename string) (*Signature, error) {
	var sig Signature

	buf, err := ReadBase64(filename)
	if err != nil {
		return nil, fmt.Errorf("%w", err)
	}

	r := bytes.NewReader(buf)
	err = binary.Read(r, binary.BigEndian, &sig)
	if err != nil {
		return nil, fmt.Errorf("%w", err)
	}

	return &sig, nil
}

// WriteBase64 encodes data in base64 and writes it the file given in
// filename. If overwrite is true it overwrites any existing file,
// otherwise it returns an error.
func WriteBase64(filename string, data any, comment string, overwrite bool) error {
	var buf bytes.Buffer

	err := binary.Write(&buf, binary.BigEndian, data)
	if err != nil {
		return fmt.Errorf("%w", err)
	}
	b64 := base64.StdEncoding.EncodeToString(buf.Bytes())
	b64 += "\n"

	var f *os.File

	f, err = os.OpenFile(filename, os.O_RDWR|os.O_CREATE|os.O_EXCL, 0o666)
	if err != nil {
		if os.IsExist(err) && overwrite {
			f, err = os.OpenFile(filename, os.O_RDWR|os.O_CREATE, 0o666)
			if err != nil {
				return fmt.Errorf("%w", err)
			}
		} else {
			return fmt.Errorf("%w", err)
		}
	}
	defer func() { _ = f.Close() }()

	_, err = fmt.Fprintf(f, "untrusted comment: %s\n", comment)
	if err != nil {
		return fmt.Errorf("%w", err)
	}

	_, err = f.Write([]byte(b64))
	if err != nil {
		return fmt.Errorf("%w", err)
	}

	return nil
}
