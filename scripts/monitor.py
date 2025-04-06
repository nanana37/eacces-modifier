import csv
import argparse

# Set up argument parser
parser = argparse.ArgumentParser(description="Process log and CSV files.")
parser.add_argument("csv_file", help="Path to the input CSV file")
parser.add_argument("log_file", help="Path to the input log file")
args = parser.parse_args()

# Read CSV: Sort by ID for each function and store
csv_entries = {}
with open(args.csv_file, newline='') as f:
    reader = csv.DictReader(f)
    for row in reader:
        key = (row["File"].strip(), row["Function"].strip())
        csv_entries.setdefault(key, {})
        csv_entries[key][int(row["ID"])] = row  # Convert ID to int and use it

# Read logs (format: [hoge],filename,func,retval,flagA,flagB)
with open(args.log_file) as f:
    for line in f:
        # Split log line by commas
        parts = line.strip().split(",")
        if len(parts) != 6:
            continue

        # Assign each part to a variable
        _, file, func, retval, flagA_str, flagB_str = parts
        key = (file.strip(), func.strip())

        # If the key does not exist in the CSV
        if key not in csv_entries:
            print(f"Key not found in CSV: {key}")
            continue

        try:
            # Read flags as hexadecimal
            flagA = int(flagA_str, 16)
            flagB = int(flagB_str, 16)
        except ValueError:
            # Skip if the flags are invalid
            continue

        # Check IDs from 0 to 63 (extend as needed)
        for i in range(64):
            # Skip if the corresponding bit in flagA is 0
            if not ((flagA >> i) & 1):
                continue

            # Retrieve the CSV entry
            entry = csv_entries[key].get(i)
            if entry:
                # Output the file name and function name first
                if i == 0:
                    print(f"-- {entry['File']}::{entry['Function']}() returned {retval} --")  # Output file name and function name
                # Output line number and content
                if entry['EventType'] == "if":
                    print(f"[#{entry['Line']}] {entry['Content']} ({'True' if ((flagB >> i) & 1) else 'False'})")
                elif entry['EventType'] == "switch":
                    print(f"[#{entry['Line']}] {entry['Content']} (switch)")