# DriveEase - Vehicle Rental System

A beginner-friendly full-stack vehicle rental website using:
- HTML
- CSS
- JavaScript
- C++ (Winsock HTTP server)
- CSV files for simple local storage

## Requirements
Windows + VS Code + MSYS2/MinGW g++.

## Run
1. Open this folder in VS Code.
2. Open the VS Code terminal.
3. Compile:
   g++ server.cpp -o server.exe -lws2_32 -std=c++20
4. Run:
   .\server.exe
5. Open your browser:
   http://localhost:8080

## Features
- Vehicle listing and search
- Vehicle type filter
- User registration
- User login
- Booking form
- Rental price calculation
- My Bookings
- C++ HTTP server
- Local CSV data storage

## Data files
data/vehicles.csv = vehicle inventory
data/users.csv = registered users
data/bookings.csv = bookings

IMPORTANT:
This is an educational project. Passwords are stored as plain text in users.csv.
For a real production system, use password hashing, HTTPS, proper sessions,
input validation, a real database, and an established web framework/library.
