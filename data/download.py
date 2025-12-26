#!/usr/bin/env python3
"""
Download hypergraph data from Google Drive
"""

import os
import requests
from pathlib import Path
from urllib.parse import urlparse, parse_qs

DOWNLOAD_LINKS = {
    "CH": "https://drive.usercontent.google.com/download?id=163ehVtR-HGVA-wmH88AJ2U2ev4bv2TG6&export=download&authuser=0",
    "CP": "https://drive.usercontent.google.com/download?id=1H7PGDPvjCyxbogUqw17YgzMc_GHLjbZA&export=download&authuser=0",
    "SB": "https://drive.usercontent.google.com/download?id=1DDrJO5fwDGuvMfnnGvrV_m6fA_YHojKF&export=download&authuser=0",
    "HB": "https://drive.usercontent.google.com/download?id=1-qiSw7YPfiTzJlA73MEH6jlxibHwlhFr&export=download&authuser=0",
    "WT": "https://drive.usercontent.google.com/download?id=1kl6wuvopJ5_wvEIy6YnwoXh1SjlIpRhu&export=download&authuser=0",
    "TC": "https://drive.usercontent.google.com/download?id=1Mcl28gC0YiQF0NOtWhobhvJU634c04xJ&export=download&authuser=0",
    "CD": "https://drive.usercontent.google.com/download?id=1tC0TdzV_IMTzhkIN_4P8y9M1a3lH-ScK&export=download&authuser=0&confirm=t&uuid=43aa41f8-08a8-4747-82ad-a4942763e47f&at=ANTm3cy4cW1RrJJa9_bS9JOux0Rb%3A1766728349263",
    "AR": "https://drive.usercontent.google.com/download?id=1tC0TdzV_IMTzhkIN_4P8y9M1a3lH-ScK&export=download&authuser=0&confirm=t&uuid=43aa41f8-08a8-4747-82ad-a4942763e47f&at=ANTm3cy4cW1RrJJa9_bS9JOux0Rb%3A1766728349263",
}

def get_filename_from_response(response, default_name):
    content_disposition = response.headers.get('Content-Disposition', '')
    if content_disposition:
        import re
        filename_match = re.search(r'filename[^;=\n]*=(([\'"]).*?\2|[^\s;]+)', content_disposition)
        if filename_match:
            filename = filename_match.group(1).strip('\'"')
            from urllib.parse import unquote
            filename = unquote(filename)
            return filename
    return default_name

def download_file(url, default_filename, output_dir="."):

    output_path = Path(output_dir) / default_filename
    
    if output_path.exists():
        return True
    
    
    try:
        headers = {
            'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36'
        }
        
        session = requests.Session()
        response = session.get(url, headers=headers, stream=True, timeout=30)
        response.raise_for_status()
        
        actual_filename = get_filename_from_response(response, default_filename)
        if actual_filename != default_filename:
            output_path = Path(output_dir) / actual_filename

            if output_path.exists():
                return True
        
        total_size = int(response.headers.get('content-length', 0))
        
        downloaded = 0
        with open(output_path, 'wb') as f:
            for chunk in response.iter_content(chunk_size=8192):
                if chunk:
                    f.write(chunk)
                    downloaded += len(chunk)
                    if total_size > 0:
                        percent = (downloaded / total_size) * 100

        return True
        
    except requests.exceptions.RequestException as e:
        if output_path.exists():
            output_path.unlink()
        return False

def main():

    script_dir = Path(__file__).parent
    output_dir = script_dir
    
    print("=" * 60)

    
    success_count = 0
    fail_count = 0
    

    for name, url in DOWNLOAD_LINKS.items():

        parsed_url = urlparse(url)
        query_params = parse_qs(parsed_url.query)
        file_id = query_params.get('id', [name])[0]
        
        default_filename = f"{name}.dat"  
        
        if download_file(url, default_filename, output_dir):
            success_count += 1
        else:
            fail_count += 1
        print() 
    
    print("=" * 60)


if __name__ == "__main__":
    main()

