#!/usr/bin/env python3
"""
Python to C++ Currency Analyzer Integration
------------------------------------------
This script uses currencyRC2.py to get data and passes it to the C++ analyzer
"""

import os
import sys
import subprocess
import time
import argparse
from datetime import datetime

def list_json_files(directory):
    """List all JSON files in the directory."""
    json_files = [f for f in os.listdir(directory) if f.endswith('.json')]
    return sorted(json_files)

def select_json_file(directory):
    """Present a menu to select a JSON file."""
    json_files = list_json_files(directory)
    
    if not json_files:
        print("[Python] No JSON files found in directory.")
        return None
    
    print("\n=== Available JSON Files ===")
    for i, filename in enumerate(json_files):
        file_path = os.path.join(directory, filename)
        file_size = os.path.getsize(file_path) / 1024  # Size in KB
        file_time = datetime.fromtimestamp(os.path.getmtime(file_path)).strftime('%Y-%m-%d %H:%M:%S')
        print(f"{i+1}. {filename} ({file_size:.1f} KB, {file_time})")
    
    print(f"{len(json_files)+1}. [LATEST] Use most recent file")
    print(f"{len(json_files)+2}. [ALL] Analyze all files")
    
    try:
        choice = int(input("\nSelect a file (number): "))
        if choice == len(json_files) + 1:
            # User selected "latest"
            return os.path.join(directory, json_files[-1])
        elif choice == len(json_files) + 2:
            # User selected "all"
            return [os.path.join(directory, f) for f in json_files]
        elif 1 <= choice <= len(json_files):
            # User selected a specific file
            return os.path.join(directory, json_files[choice-1])
        else:
            print("[Python] Invalid selection, using the most recent file.")
            return os.path.join(directory, json_files[-1])
    except (ValueError, IndexError):
        print("[Python] Invalid input, using the most recent file.")
        return os.path.join(directory, json_files[-1])

def main():
    parser = argparse.ArgumentParser(description="Currency Analyzer Python-C++ Integration")
    parser.add_argument("--refresh", type=int, default=60,
                      help="Update interval in seconds")
    parser.add_argument("--mode", choices=["oneshot", "continuous", "analyze"], default="oneshot",
                      help="Run mode: oneshot, continuous, or analyze existing files")
    parser.add_argument("--output-dir", default="data",
                      help="Directory to store data files")
    parser.add_argument("--all-files", action="store_true",
                      help="Analyze all JSON files in the directory")
    
    args = parser.parse_args()
    
    # Create output directory if it doesn't exist
    os.makedirs(args.output_dir, exist_ok=True)
    
    print("=== Currency Analyzer Integration ===")
    
    if args.mode == "analyze":
        # Just analyze existing files
        if args.all_files:
            # Process all files
            json_files = list_json_files(args.output_dir)
            for json_file in json_files:
                json_path = os.path.join(args.output_dir, json_file)
                print(f"[Python] Analyzing {json_file}...")
                subprocess.run(["currency_analyzer.exe", json_path])
        else:
            # Let user select a file
            selected = select_json_file(args.output_dir)
            if isinstance(selected, list):
                # User selected "all files"
                for json_path in selected:
                    print(f"[Python] Analyzing {os.path.basename(json_path)}...")
                    subprocess.run(["currency_analyzer.exe", json_path])
            elif selected:
                print(f"[Python] Analyzing {os.path.basename(selected)}...")
                subprocess.run(["currency_analyzer.exe", selected])
    
    elif args.mode == "oneshot":
        # Run one-time data collection
        print(f"[Python] Running currencyRC2.py to fetch data...")
        subprocess.run([
            "python", "currencyRC2.py",
            "--no-dashboard",
            "--save-json",
            f"--output-dir={args.output_dir}"
        ])
        
        # Let user select which file to analyze
        selected = select_json_file(args.output_dir)
        if isinstance(selected, list):
            # User selected "all files"
            for json_path in selected:
                print(f"[Python] Analyzing {os.path.basename(json_path)}...")
                subprocess.run(["currency_analyzer.exe", json_path])
        elif selected:
            print(f"[Python] Analyzing {os.path.basename(selected)}...")
            subprocess.run(["currency_analyzer.exe", selected])
            
    elif args.mode == "continuous":
        print(f"[Python] Starting continuous mode (refresh: {args.refresh}s)...")
        try:
            while True:
                # Run data collection
                subprocess.run([
                    "python", "currencyRC2.py",
                    "--no-dashboard",
                    "--save-json",
                    f"--output-dir={args.output_dir}",
                    "--refresh", str(args.refresh)
                ])
                
                # Find and use the most recent JSON file
                json_files = list_json_files(args.output_dir)
                if json_files:
                    latest_json = json_files[-1]
                    latest_json_path = os.path.join(args.output_dir, latest_json)
                    
                    print(f"[Python] Running C++ analyzer with data from {latest_json_path}...")
                    subprocess.run(["currency_analyzer.exe", latest_json_path])
                else:
                    print("[Python] Error: No JSON files found in output directory.")
                
                print(f"[Python] Waiting {args.refresh} seconds until next update...")
                time.sleep(args.refresh)
                
        except KeyboardInterrupt:
            print("\n[Python] Continuous updates stopped by user.")

if __name__ == "__main__":
    main()