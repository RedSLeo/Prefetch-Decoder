# Prefetch Decoder & Telemetry Pipeline

## Overview
A comprehensive digital forensics pipeline designed to extract hidden execution data from Windows machines. The core engine, written in C++, interfaces directly with Windows NT kernel APIs to decompress raw Prefetch files and map their binary structures. A custom PowerShell pipeline then automates the shipment of these logs into an SQL Server database, turning raw machine data into a queryable search

## Core Features
* **Raw Binary Parsing:** Maps Windows Prefetch memory structures (Version 31) directly from raw hexadecimal data without relying on third-party forensic software.
* **System-Level Decompression:** Interfaces dynamically with undocumented NT Kernel APIs (`ntdll.dll`) to handle LZX / Xpress Huffman decompression in memory.
* **Automated Data Pipeline:** Extracts high-fidelity execution telemetry (Executable Names and File Hashes) into structured CSV logs.
* **Database Integration:** Utilizes a custom PowerShell script to ingest the extracted evidence directly into a SQL Server Management Studio (SSMS) database for centralized analysis.

## Technology Stack
* **Language:** Standard C++
* **Libraries/APIs:** Windows API (`<windows.h>`), Standard Template Library (`<vector>`, `<fstream>`)
* **Automation:** PowerShell
* **Database:** SQL Server Management Studio (SSMS)

## Architecture Workflow
1. **Ingestion:** The C++ engine (`PrefetchEngine.exe`) opens a target `.pf` file in strict binary mode to prevent data corruption.
2. **Analysis:** The engine reads the header signature (`MAM`) to determine compression status and calculates required memory workspaces.
3. **Decompression:** `RtlGetCompressionWorkSpaceSize` and `RtlDecompressBufferEx` are invoked from the Windows kernel to unpack the file into a readable state.
4. **Extraction:** Execution artifacts (names and hashes) are mapped via custom C++ structs and appended to `evidence_log.csv`.
5. **Log Shipping:** `LogShipper.ps1` reads the CSV and executes SQL queries to securely push the new evidence into the `ForensicsDB` SQL Server database.

## Project Structure
```text
📦 Prefetch-Decoder
 ┣ 📜 PrefetchEngine.cpp   # Core C++ parsing and decompression engine
 ┣ 📜 PrefetchEngine.exe   # Compiled executable
 ┣ 📜 LogShipper.ps1       # PowerShell automation for database ingestion
 ┣ 📜 ForensicsDB.sql      # SQL schema for the evidence database
 ┣ 📜 evidence_log.csv     # Intermediary structured telemetry output
 ┗ 📜 README.md
