"""
ARQ Code Generation Script

This script generates C++ code from TOML entity definitions using Jinja2 templates.
It supports multiple entity types (refdata, mktdata, etc.) and can generate code
for specific types or all types at once.
"""

import tomli
from jinja2 import Environment, FileSystemLoader
import os
import argparse
import sys
from pathlib import Path
import re
from typing import List, Dict, Optional, Tuple, Any
import json
import time


class CodeGenerationError(Exception):
    """Custom exception for code generation errors."""
    pass


class EntityDefinition:
    """Represents a single entity definition with its metadata."""
    
    def __init__(self, data: Dict[str, Any], entity_type: str, source_file: str):
        self.data = data
        self.entity_type = entity_type
        self.source_file = source_file
        self.name = data.get('name', '')
        self.key = data.get('key')
        self.key_type = None
        self.has_indices = False
        
        self._validate_and_process()
    
    def _validate_and_process(self) -> None:
        """Validate the entity definition, process key type and process indices."""
        if not self.name:
            raise CodeGenerationError(f"Entity in {self.source_file} is missing a name")
        
        # Process key type if a key is defined
        if self.key:
            for member in self.data.get('members', []):
                if member.get('name') == self.key:
                    self.key_type = member.get('type')
                    break
            
            if not self.key_type:
                raise CodeGenerationError(
                    f"Entity '{self.name}' in {self.source_file} defines key '{self.key}' "
                    f"but no matching member found"
                )
            
        # Store the processed key_type back in the data
        self.data['key_type'] = self.key_type

        # Check if the entity has indices
        for member in self.data.get('members', []):
            if member.get('index_type', 'None') != 'None':
                if member.get('type') != 'string':
                    raise CodeGenerationError( f"Entity {self.name} in {self.source_file} has specified index type on a non-string member - this is not supported")
                self.has_indices = True
                break

        # Store has_indices in template data
        self.data['has_indices'] = self.has_indices

class TemplateMetadata:
    """Represents metadata extracted from a template file."""
    
    def __init__(self, template_path: Path):
        self.template_path = template_path
        self.metadata = self._parse_metadata()
    
    def _parse_metadata(self) -> Dict[str, str]:
        """Parse metadata from template comments."""
        metadata = {}
        meta_regex = re.compile(r"//\s*(\w+):\s*(.*)")
        
        try:
            with open(self.template_path, 'r', encoding='utf-8') as f:
                in_metadata_block = False
                for line in f:
                    line = line.strip()
                    if line == "{#":
                        continue
                    if line == "// codegen-metadata":
                        in_metadata_block = True
                        continue
                    if in_metadata_block and line == "#}":
                        break
                    if in_metadata_block:
                        match = meta_regex.match(line)
                        if match:
                            key, value = match.groups()
                            metadata[key.strip()] = value.strip()
        except Exception as e:
            raise CodeGenerationError(f"Failed to parse metadata from {self.template_path}: {e}")
        
        return metadata
    
    @property
    def output_path(self) -> Optional[str]:
        """Get the output path from metadata."""
        return self.metadata.get('output_path')
    
    @property
    def is_valid(self) -> bool:
        """Check if the template has valid metadata."""
        return self.output_path is not None


class CodeGenerator:
    """Main code generator class that orchestrates the generation process."""
    
    def __init__(self, definitions_dir: Path, template_dir: Path, output_dir: Path):
        self.definitions_dir = definitions_dir
        self.template_dir = template_dir
        self.output_dir = output_dir
        self.types_data = {}
        self.entities_by_type: Dict[str, List[EntityDefinition]] = {}
        self.jinja_env = None
        # cache file to track template mtimes so we only regenerate changed outputs
        self.cache_file = Path(__file__).parent.resolve() / '.codegen_cache.json'
        self._cache: Dict[str, Any] = {}
        self._load_cache()
    
    def get_available_entity_types(self) -> List[str]:
        """Get a list of available entity types from the definitions directory."""
        entity_types = []
        try:
            for filename in os.listdir(self.definitions_dir):
                if filename.endswith('_entities.toml'):
                    entity_type = filename.replace('_entities.toml', '')
                    entity_types.append(entity_type)
        except OSError as e:
            raise CodeGenerationError(f"Failed to list definitions directory: {e}")
        
        return sorted(entity_types)
    
    def load_types_definition(self) -> None:
        """Load the base types definition from types.toml."""
        types_path = self.definitions_dir / 'types.toml'
        print(f"Loading base types from: {types_path}")
        
        try:
            with open(types_path, 'rb') as f:
                self.types_data = tomli.load(f)
        except Exception as e:
            raise CodeGenerationError(f"Could not load 'types.toml': {e}")
    
    def load_entity_definitions(self) -> None:
        """Load all entity definitions from TOML files."""
        print(f"Loading entity definitions from: {self.definitions_dir}")
        
        try:
            filenames = sorted(os.listdir(self.definitions_dir))
        except OSError as e:
            raise CodeGenerationError(f"Failed to list definitions directory: {e}")
        
        for filename in filenames:
            if filename == 'types.toml' or not filename.endswith('.toml'):
                continue
            
            filepath = self.definitions_dir / filename
            print(f"  - Processing {filepath.name}...")
            
            # Extract entity type from filename
            entity_type = filename.replace('_entities.toml', '')
            
            try:
                with open(filepath, 'rb') as f:
                    entity_file_data = tomli.load(f)
            except Exception as e:
                raise CodeGenerationError(f"Failed to load {filepath}: {e}")
            
            if 'entities' not in entity_file_data:
                continue
            
            # Process entities for this type
            entities = []
            for entity_data in entity_file_data['entities']:
                try:
                    entity = EntityDefinition(entity_data, entity_type, filepath.name)
                    entities.append(entity)
                except CodeGenerationError as e:
                    raise CodeGenerationError(f"Error processing entity in {filepath.name}: {e}")
            
            self.entities_by_type[entity_type] = entities
    
    def setup_jinja_environment(self) -> None:
        """Set up the Jinja2 environment for template rendering."""
        self.jinja_env = Environment(
            loader=FileSystemLoader(self.template_dir),
            trim_blocks=True,
            lstrip_blocks=True
        )
        
        # Add custom filters
        self.jinja_env.filters['capitalise_first'] = self._capitalise_first_filter
        self.jinja_env.filters['pascal_case'] = self._pascal_case_filter
        self.jinja_env.filters['camel_case'] = self._camel_case_filter
        self.jinja_env.filters['snake_case'] = self._snake_case_filter
    
    def _capitalise_first_filter(self, text: str) -> str:
        """Jinja2 filter to capitalize the first letter of a string."""
        if not text:
            return text
        return text[0].upper() + text[1:]
    
    def _pascal_case_filter(self, text: str) -> str:
        """Jinja2 filter to convert text to PascalCase."""
        if not text:
            return text
        # Split on underscores, spaces, or hyphens and capitalize each word
        words = re.split(r'[_\s-]+', text)
        return ''.join(word.capitalize() for word in words if word)
    
    def _camel_case_filter(self, text: str) -> str:
        """Jinja2 filter to convert text to (lower)camelCase."""
        if not text:
            return text
        pascal_case = self._pascal_case_filter(text)
        return pascal_case[0].lower() + pascal_case[1:] if pascal_case else pascal_case
    
    def _snake_case_filter(self, text: str) -> str:
        """Jinja2 filter to convert text to snake_case."""
        if not text:
            return text
        
        # Handle sequences of consecutive uppercase letters followed by lowercase
        # This converts "FXRate" to "FX_Rate" first
        text = re.sub('([A-Z]+)([A-Z][a-z])', r'\1_\2', text)
        
        # Insert underscore before uppercase letters that follow lowercase letters
        # This converts "FX_Rate" to "FX_rate" and handles cases like "myVariable"
        text = re.sub('([a-z])([A-Z])', r'\1_\2', text)
        
        return text.lower()
    
    def generate_for_entity_type(self, entity_type: str) -> None:
        """Generate code for a specific entity type."""
        if entity_type not in self.entities_by_type:
            available_types = list(self.entities_by_type.keys())
            raise CodeGenerationError(
                f"Entity type '{entity_type}' not found. Available types: {available_types}"
            )
        
        entities = self.entities_by_type[entity_type]
        print(f"\nProcessing templates for entity type: {entity_type}")
        
        # Check if template directory exists for this entity type
        entity_template_dir = self.template_dir / entity_type
        if not entity_template_dir.exists():
            print(f"  - No template directory found for {entity_type} (expected: {entity_template_dir})")
            return
        
        # Prepare data model for templates
        entity_data_model = {
            'types': self.types_data.get('types', {}),
            'entities': [entity.data for entity in entities],
            'entities_by_type': {et: [e.data for e in ents] for et, ents in self.entities_by_type.items()},
            'current_entity_type': entity_type,
        }
        
        # Find and process all .j2 templates
        template_paths = list(entity_template_dir.rglob('*.j2'))
        if not template_paths:
            print(f"  - No templates found in {entity_template_dir}")
            return
        
        for template_path in template_paths:
            self._process_template(template_path, entity_data_model)
    
    def _process_template(self, template_path: Path, data_model: Dict[str, Any]) -> None:
        """Process a single template file."""
        relative_template_path = template_path.relative_to(self.template_dir).as_posix()
        
        try:
            metadata = TemplateMetadata(template_path)
            
            if not metadata.is_valid:
                print(f"  - Skipping {relative_template_path} (no 'output_path' defined in metadata)")
                return
            # decide whether to regenerate based on template mtime and cache
            try:
                template_mtime = template_path.stat().st_mtime
            except OSError:
                template_mtime = time.time()

            cache_entry = self._cache.get(relative_template_path, {})
            cached_mtime = cache_entry.get('mtime', 0)

            output_path = self.output_dir / metadata.output_path

            if template_mtime <= cached_mtime and output_path.exists():
                print(f"  (-) Skipping {relative_template_path} (template unchanged, output exists)")
                return

            template = self.jinja_env.get_template(relative_template_path)
            output_content = template.render(data_model)

            # Write the output file
            output_path.parent.mkdir(parents=True, exist_ok=True)

            print(f"  (+) Writing {output_path}")
            with open(output_path, 'w', encoding='utf-8', newline='\n') as f:
                f.write(output_content)

            # update cache for this template
            self._cache[relative_template_path] = {
                'mtime': template_mtime,
                'output_path': metadata.output_path,
                'written_at': time.time()
            }
            self._save_cache()
                
        except Exception as e:
            raise CodeGenerationError(f"Failed to process template {template_path}: {e}")
    
    def generate_all(self, target_entity_type: Optional[str] = None) -> None:
        """Generate code for all entity types or a specific one."""
        print("\n--- Generating Files ---")
        
        if target_entity_type:
            print(f"Filtering to entity type: {target_entity_type}")
            self.generate_for_entity_type(target_entity_type)
        else:
            print("Generating for all entity types")
            for entity_type in self.entities_by_type.keys():
                self.generate_for_entity_type(entity_type)
        
        print("\nCode generation complete.")

    def _load_cache(self) -> None:
        """Load the template mtime cache from disk (silent if missing)."""
        try:
            if self.cache_file.exists():
                with open(self.cache_file, 'r', encoding='utf-8') as f:
                    self._cache = json.load(f)
            else:
                self._cache = {}
        except Exception as e:
            raise CodeGenerationError(f"Failed to read cache file {self.cache_file}: {e}")

    def _save_cache(self) -> None:
        """Persist the template mtime cache to disk."""
        try:
            # ensure output dir exists before writing cache
            self.output_dir.mkdir(parents=True, exist_ok=True)
            with open(self.cache_file, 'w', encoding='utf-8') as f:
                json.dump(self._cache, f, indent=2)
        except Exception as e:
            raise CodeGenerationError(f"Failed to write cache file {self.cache_file}: {e}")


def main():
    """Main entry point for the code generation script."""
    parser = argparse.ArgumentParser(
        description="ARQ Code Generation Script - Generate C++ code from TOML entity definitions"
    )
    parser.add_argument(
        '--definitions-dir', 
        required=True, 
        type=Path, 
        help="Path to the directory containing TOML definitions"
    )
    parser.add_argument(
        '--template-dir', 
        required=True, 
        type=Path, 
        help="Root path to the directory containing Jinja2 templates"
    )
    parser.add_argument(
        '--output-dir', 
        required=True, 
        type=Path, 
        help="Root path to the project source code directory for placing generated files"
    )
    parser.add_argument(
        '--entity-type', 
        type=str, 
        help="Generate code for a specific entity type (e.g., 'refdata', 'mktdata'). "
             "If not specified, generates for all entity types"
    )
    parser.add_argument(
        '--list-types', 
        action='store_true', 
        help="List available entity types and exit"
    )
    
    args = parser.parse_args()
    
    try:
        generator = CodeGenerator(args.definitions_dir, args.template_dir, args.output_dir)
        
        # Handle list types option
        if args.list_types:
            available_types = generator.get_available_entity_types()
            if available_types:
                print("Available entity types:")
                for entity_type in available_types:
                    print(f"  - {entity_type}")
            else:
                print("No entity types found in the definitions directory.")
            return
        
        # Load definitions and generate code
        generator.load_types_definition()
        generator.load_entity_definitions()
        generator.setup_jinja_environment()
        generator.generate_all(args.entity_type)
        
    except CodeGenerationError as e:
        print(f"FATAL ERROR: {e}", file=sys.stderr)
        sys.exit(1)
    except KeyboardInterrupt:
        print("\nOperation cancelled by user.", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"UNEXPECTED ERROR: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()