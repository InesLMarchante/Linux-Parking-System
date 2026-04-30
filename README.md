# Linux-Parking-System
A Linux-based parking management system implementing a client-server architecture in C with IPC (FIFOs, Signals) and administrative Bash scripts

# Parking Management System

A comprehensive parking lot management system developed for the Operating Systems course at ISCTE-IUL (2024/2025). 

## Project Structure

The project is organized into two main components:

* **`bash-scripts/`**: Administrative tools for data validation, automated maintenance, and reporting.
* **`c-client-server/`**: A concurrent system implementing a client-server architecture.

---

## Part 1: Administrative Scripts (Bash)

This module focuses on data integrity and automation using shell scripting.

### Key Features:
* [cite_start]**Data Validation**: Ensures parking records match country-specific license plate formats and valid timestamps[cite: 2].
* [cite_start]**Automated Maintenance**: Uses "cron" to archive completed parking records into monthly files[cite: 1, 2].
* **Statistical Reporting**: Generates dynamic HTML reports with metrics like Top 3 longest stays and occupancy by country.
* [cite_start]**Management Menu**: A centralized interface to invoke all system functionalities[cite: 3].

**Technologies used:** "awk", "sed", "grep", "regex", "crontab".

---

## Part 2: Client-Server System (C)

A high-performance concurrent system that manages real-time parking entries and exits. This module represents the final iteration of the project, completely eliminating basic I/O in favor of advanced System V Inter-Process Communication (IPC).

### Core Files:
* "servidor.c": Initializes IPC resources, spawns Dedicated Server child processes ("fork()") for each client request, and handles system shutdowns.
* "cliente.c": Simulates user interactions, sending parking requests to the server, and handling timeouts or user cancellations (Ctrl+C).
* "common.h" & "defines.h": Contains shared data structures, function prototypes, and the `IPC_KEY` configuration.

### Technical Highlights:
* **System V Message Queues ("msgget", "msgsnd", "msgrcv")**: Implemented bidirectional communication replacing standard FIFOs. Differentiates messages by type (Main Server vs. Dedicated Servers vs. Clients).
* **Shared Memory ("shmget", "shmat")**: Maintains the real-time state of the parking lot grid, allowing concurrent access across multiple running processes without constant disk reads.
* **Semaphores ("semget", "semop")**: Critical section management. Uses semaphores to provide strict synchronization and prevent race conditions when reading/writing to Shared Memory and the "estacionamentos.txt" log file.
* **Advanced Signal Handling**: Termination routines using "SIGINT" to safely clean up IPC resources and "SIGCHLD" to prevent zombie processes during concurrent execution.
---

## How to Run

### Prerequisites:
* Linux Environment
* GCC Compiler

### Steps:
1. **Compile the C system:**
   ```bash
   cd c-client-server
   gcc servidor.c -o servidor
   gcc cliente.c -o cliente
