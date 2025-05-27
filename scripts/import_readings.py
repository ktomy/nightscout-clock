#!/usr/bin/env python3
# filepath: /home/ktomy/projects/NSClock/scripts/import_readings.py
# Script to import glucose readings from a CSV file and send them to NSClock's API
# Usage: python3 import_readings.py <csv_file_path>

import sys
import csv
import json
import requests
from datetime import datetime, timedelta
from requests_toolbelt.utils import dump
import argparse

# Define constants
NSCLOCK_URL = "http://nsclock.lan/"
API_ENDPOINT = "api/v1/entries"
HEADERS = {
    "Content-Type": "application/json"
}

# Define trend mapping from text to number
TREND_MAPPING = {
    "Flat": 4,
    "FortyFiveUp": 3,
    "FortyFiveDown": 5,
    "SingleUp": 2,
    "SingleDown": 6,
    "DoubleUp": 1,
    "DoubleDown": 7,
    "None": 0
}

def read_csv_data(file_path):
    """Read glucose readings from CSV file"""
    readings = []
    try:
        with open(file_path, 'r') as csvfile:
            reader = csv.DictReader(csvfile)
            for row in reader:
                # Remove whitespace from keys
                clean_row = {k.strip(): v.strip() for k, v in row.items()}
                readings.append(clean_row)
        return readings
    except Exception as e:
        print(f"Error reading CSV file: {e}")
        sys.exit(1)

def convert_to_entries(readings):
    """Convert CSV readings to API format"""
    now = datetime.now()
    entries = []
    
    for reading in readings:
        # Get seconds ago value
        seconds_ago = int(reading.get("Seconds Ago", 0))
        entry_time = now - timedelta(seconds=seconds_ago)
        
        # Get trend value
        trend_text = reading.get("Trend", "Flat")
        trend_value = TREND_MAPPING.get(trend_text, 4)  # Default to Flat (4) if not found
        
        entry = {
            "dateString": entry_time.strftime("%Y-%m-%dT%H:%M:%S"),
            "date": int(entry_time.timestamp()) * 1000,
            "sgv": int(reading.get("Reading", 0)),
            "trend": trend_value,
            "direction": trend_text
        }
        entries.append(entry)
    
    return entries

def clear_readings():
    """Clear all existing readings from the NSClock API"""
    try:
        print(f"Clearing all readings from {NSCLOCK_URL + API_ENDPOINT}")
        response = requests.delete(NSCLOCK_URL + API_ENDPOINT, timeout=5)
        
        if response.status_code == 200:
            print("Data cleared successfully")
            return True
        else:
            print(f"Failed to clear data. Status code: {response.status_code}")
            print(dump.dump_all(response).decode('utf-8'))
            return False
    except Exception as e:
        print(f"Error clearing data: {e}")
        return False

def send_readings(entries):
    """Send readings to NSClock API"""
    try:
        print(f"Sending {len(entries)} readings to {NSCLOCK_URL + API_ENDPOINT}")
        for entry in entries:
            print(f"  - sgv: {entry['sgv']}, dateString: {entry['dateString']}, trend: {entry['direction']}")
        
        response = requests.post(NSCLOCK_URL + API_ENDPOINT, json=entries, headers=HEADERS, timeout=5)
        
        if response.status_code == 200:
            print("Data sent successfully")
            return True
        else:
            print(f"Failed to send data. Status code: {response.status_code}")
            print(dump.dump_all(response).decode('utf-8'))
            return False
    except Exception as e:
        print(f"Error sending data: {e}")
        return False

def main():
    """Main function"""
    parser = argparse.ArgumentParser(description='Import glucose readings from CSV to NSClock API')
    parser.add_argument('file_path', nargs='?', help='Path to the CSV file with readings')
    parser.add_argument('--clear', '-c', action='store_true', help='Clear existing readings before import')
    parser.add_argument('--delete-only', '-d', action='store_true', help='Only delete existing readings, do not import')
    
    args = parser.parse_args()
    
    # If --delete-only is specified, just clear the readings and exit
    if args.delete_only:
        if clear_readings():
            print("Readings cleared successfully")
            sys.exit(0)
        else:
            print("Failed to clear readings")
            sys.exit(1)
    
    # Make sure we have a file path if not in delete-only mode
    if not args.file_path:
        parser.print_help()
        sys.exit(1)
    
    # Clear existing readings if requested
    if args.clear:
        if not clear_readings():
            print("Failed to clear existing readings")
            sys.exit(1)
    
    # Read CSV data
    readings = read_csv_data(args.file_path)
    
    if not readings:
        print("No readings found in the CSV file")
        sys.exit(1)
        
    print(f"Found {len(readings)} readings in the CSV file")
    
    # Convert to entries format
    entries = convert_to_entries(readings)
    
    # Send readings to NSClock API
    if send_readings(entries):
        print("Process completed successfully")
    else:
        print("Process failed")
        sys.exit(1)

if __name__ == "__main__":
    main()
