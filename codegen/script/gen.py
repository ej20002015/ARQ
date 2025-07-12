import tomli
from jinja2 import Environment, FileSystemLoader
import os
import argparse
import sys
from pathlib import Path
import re

def parse_metadata(template_path: Path) -> dict:
    """Parses metadata like 'output_path' from comments at the top of a template."""
    metadata = {}
    # Regex to find lines like: // key: value
    meta_regex = re.compile(r"//\s*(\w+):\s*(.*)")
    
    with open(template_path, 'r', encoding='utf-8') as f:
        in_metadata_block = False
        for line in f:
            line = line.strip()
            if line == "{#": # Start of Jinja2 comment block
                continue
            if line == "// codegen-metadata":
                in_metadata_block = True
                continue
            if in_metadata_block and line == "#}": # End of Jinja2 comment block
                break
            if in_metadata_block:
                match = meta_regex.match(line)
                if match:
                    key, value = match.groups()
                    metadata[key.strip()] = value.strip()
    return metadata

def main():
    """
    Main function to drive the code generation process.
    Reads data definitions from TOML files and renders Jinja2 templates.
    """
    parser = argparse.ArgumentParser(description="TMQ Code Generation Script")
    parser.add_argument('--definitions-dir', required=True, type=Path, help="Path to the directory containing TOML definitions.")
    parser.add_argument('--template-dir', required=True, type=Path, help="Root path to the directory containing Jinja2 templates.")
    parser.add_argument('--output-dir', required=True, type=Path, help="Root path to the project source code directory for placing generated files.")
    args = parser.parse_args()

    # --- 1. Load Data Definitions (No change) ---
    types_path = args.definitions_dir / 'types.toml'
    print(f"Loading base types from: {types_path}")
    try:
        with open(types_path, 'rb') as f:
            types_data = tomli.load(f)
    except Exception as e:
        print(f"FATAL ERROR: Could not load 'types.toml'.\n{e}", file=sys.stderr)
        sys.exit(1)

    all_entities = []
    print(f"Loading entity definitions from: {args.definitions_dir}")
    for filename in sorted(os.listdir(args.definitions_dir)):
        if filename == 'types.toml' or not filename.endswith(".toml"):
            continue
        
        filepath = args.definitions_dir / filename
        print(f"  - Processing {filepath.name}...")
        with open(filepath, 'rb') as f:
            entity_file_data = tomli.load(f)
            if 'entities' in entity_file_data:
                for entity in entity_file_data['entities']:
                    key = entity.get('key')
                    for member in entity.get('members', []):
                        if member.get('name') == key:
                            entity['key_type'] = member.get('type')
                            break
                    if not entity.get('key_type'):
                        print(f"FATAL ERROR: Entity '{entity.get('name')}' in {filepath.name} does not have a valid key type.", file=sys.stderr)
                        sys.exit(1)
                        
                all_entities.extend(entity_file_data['entities'])

    master_data_model = {
        'types': types_data.get('types', {}),
        'entities': all_entities,
    }

    # --- 2. Setup Jinja2 Environment (No change) ---
    env = Environment(loader=FileSystemLoader(args.template_dir), trim_blocks=True, lstrip_blocks=True)

    # --- 3. Discover, Render, and Write Files (New Logic) ---
    print("\n--- Generating Files ---")
    
    # Use rglob to find all .j2 files recursively in the template directory
    for template_path in args.template_dir.rglob('*.j2'):
        # Get path relative to the template directory for Jinja2's loader
        relative_template_path = template_path.relative_to(args.template_dir).as_posix()
        
        metadata = parse_metadata(template_path)
        
        if 'output_path' not in metadata:
            print(f"  - Skipping {relative_template_path} (no 'output_path' defined in metadata)")
            continue

        template = env.get_template(relative_template_path)
        output_content = template.render(master_data_model)
        
        # Determine the full output path
        output_path = args.output_dir / metadata['output_path']
        
        # Create the destination directory if it doesn't exist
        output_path.parent.mkdir(parents=True, exist_ok=True)
        
        print(f"  -> Writing {output_path}")
        with open(output_path, 'w', encoding='utf-8', newline='\n') as f:
            f.write(output_content)
    
    print("\nCode generation complete.")

if __name__ == "__main__":
    main()