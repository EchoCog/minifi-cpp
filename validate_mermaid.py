#!/usr/bin/env python3
"""
Simple Mermaid diagram validation script for ARCHITECTURE.md
"""
import re
import sys

def validate_mermaid_syntax(filename):
    """Validate basic Mermaid syntax in the file"""
    with open(filename, 'r') as f:
        content = f.read()
    
    # Find mermaid blocks
    mermaid_pattern = r'```mermaid\n(.*?)\n```'
    blocks = re.findall(mermaid_pattern, content, re.DOTALL)
    
    print(f"Found {len(blocks)} Mermaid diagram(s) in {filename}")
    
    valid_types = [
        'graph', 'flowchart', 'sequenceDiagram', 'classDiagram', 
        'stateDiagram', 'mindmap', 'pie', 'gantt', 'gitgraph'
    ]
    
    errors = 0
    for i, block in enumerate(blocks, 1):
        lines = [line.strip() for line in block.strip().split('\n') if line.strip()]
        if not lines:
            print(f"  ✗ Diagram {i}: Empty diagram")
            errors += 1
            continue
            
        first_line = lines[0]
        if any(first_line.startswith(t) for t in valid_types):
            print(f"  ✓ Diagram {i}: {first_line}")
        else:
            print(f"  ✗ Diagram {i}: Unknown type - {first_line}")
            errors += 1
    
    return errors == 0

if __name__ == "__main__":
    filename = sys.argv[1] if len(sys.argv) > 1 else "ARCHITECTURE.md"
    success = validate_mermaid_syntax(filename)
    sys.exit(0 if success else 1)