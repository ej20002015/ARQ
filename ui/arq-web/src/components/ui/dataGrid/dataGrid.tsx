import { useMemo } from 'react';
import { AgGridReact, type AgGridReactProps } from 'ag-grid-react';
import { ModuleRegistry, AllCommunityModule, themeQuartz, iconSetAlpine } from 'ag-grid-community';

import './dataGrid.css';

const theme2 = themeQuartz
    .withPart(iconSetAlpine)
    .withParams({
        accentColor: "#7C3AED",
        backgroundColor: "#000000",
        borderRadius: 2,
        browserColorScheme: "dark",
        chromeBackgroundColor: {
            ref: "foregroundColor",
            mix: 0.07,
            onto: "backgroundColor"
        },
        columnBorder: true,
        fontFamily: [
            "-apple-system",
            "BlinkMacSystemFont",
            "Segoe UI",
            "Roboto",
            "Oxygen-Sans",
            "Ubuntu",
            "Cantarell",
            "Helvetica Neue",
            "sans-serif"
        ],
        fontSize: 12,
        foregroundColor: "#FFFFFF",
        headerBackgroundColor: "#7B39ED",
        headerFontWeight: 700,
        oddRowBackgroundColor: "#111111",
        headerRowBorder: true,
        rowBorder: true,
        spacing: 4,
        wrapperBorder: true,
        wrapperBorderRadius: 0
    });

// Register modules exactly once for the entire application
ModuleRegistry.registerModules([AllCommunityModule]);

// We extend the default AG Grid props so you can still pass anything to it
interface DataGridProps extends AgGridReactProps {
  // Can add custom ARQ-specific props here if needed
}

export function DataGrid(props: DataGridProps) {
    // Define default column settings that apply to all
    const defaultColDef = useMemo(() => ({
        sortable: true,
        filter: true,
        resizable: true,
        
        // Merge any defaultColDefs passed in by the parent
        ...props.defaultColDef, 
    }), [props.defaultColDef]);

    return (
    <div className="arq-grid-wrapper">
        <div className="h-full w-full">
            <AgGridReact
                theme={theme2}
                {...props}
                defaultColDef={defaultColDef}
                rowSelection={props.rowSelection || "single"}
                animateRows={props.animateRows !== undefined ? props.animateRows : true}
                pagination={props.pagination !== undefined ? props.pagination : true}
            />
        </div>
    </div>
    );
}