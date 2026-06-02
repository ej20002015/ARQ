import { useState, useEffect, useMemo } from "react";
import { type ColDef, ModuleRegistry, AllCommunityModule } from "ag-grid-community";
import { DataGrid } from "@/components/ui/dataGrid/dataGrid";

ModuleRegistry.registerModules([AllCommunityModule]);

export function RefDataGrid() {
    // --- 2. REACT STATE & LOGIC ---

    // Track which entity the user selected in the dropdown (defaults to User)
    const [entityType, setEntityType] = useState<string>("User");

    // Track the list of available entities from the API
    const [availableEntities, setAvailableEntities] = useState<string[]>([]);

    // Track the actual rows to feed into AG Grid
    const [rowData, setRowData] = useState<any[]>([]);

    // NEW: Track whether we are currently fetching data
    const [isLoading, setIsLoading] = useState<boolean>(false);

    // API calls

    useEffect(() => {
        const fetchEntities = async () => {
            try {
                const response = await fetch("http://localhost:5001/api/refdata/entities");
                if (!response.ok)
                    throw new Error(`Failed to fetch entities: ${response.status}`);

                const data = await response.json();
                setAvailableEntities(data);
                
                // safety check: If "User" isn't in the DB, default to the first available entity
                if (data.length > 0 && !data.includes(entityType))
                    setEntityType(data[0]);
            } catch (error) {
                console.error("Failed to load entity list:", error);
            }
        };

        fetchEntities();
    }, []);
    
    useEffect(() => {
        // 1. Define an async function inside the effect
        const fetchRefData = async () => {
            setIsLoading(true); // Turn on loading state
            setRowData([]);     // Clear the old grid data immediately
            
            try {
                // Construct the URL based on the dropdown selection
                const endpoint = `http://localhost:5001/api/refdata/records/${entityType.toLowerCase()}`;

                // 2. Make the network request
                const response = await fetch(endpoint);
                
                if (!response.ok) {
                throw new Error(`Gateway returned status: ${response.status}`);
                }

                // 3. Parse the JSON and update the grid
                const data = await response.json();
                setRowData(data);

            } catch (error) {
                console.error("Failed to fetch from ARQ.Gateway:", error);
                // In a production app, you might set an error state here to show a toast notification
            } finally {
                setIsLoading(false); // Turn off loading state whether it succeeded or failed
            }
        };

        // 4. Execute the function
        fetchRefData();
        
    }, [entityType]);

    // Define columns dynamically based on the selected entity
    const colDefs = useMemo<ColDef[]>(() => {

        // These columns are common to ALL RefData entities
        const commonHeaders: ColDef[] = [
        { field: "header.isActive", headerName: "IsActive", width: 90 },
        { field: "header.version", headerName: "Version", width: 80 },
        { field: "header.lastUpdatedBy", headerName: "LastUpdatedBy", width: 120 },
        { field: "header.lastUpdatedTs", headerName: "LastUpdatedTs", flex: 1 }
        ];

        if (entityType === "User") {
        return [
            { field: "data.userID", headerName: "UserID", width: 120 },
            { field: "data.fullName", headerName: "FullName", width: 150 },
            { field: "data.tradingDesk", headerName: "TradingDesk", width: 120 },
            { field: "data.email", headerName: "Email", width: 200 },
            ...commonHeaders // Spread operator to append the common headers at the end
        ];
        } 
        
        if (entityType === "Currency") {
        return [
            { field: "data.ccyID", headerName: "CcyID", width: 100 },
            { field: "data.name", headerName: "Name", width: 150 },
            { field: "data.decimalPlaces", headerName: "Decimal Places", width: 100 },
            { field: "data.settlementDays", headerName: "Settlement Days", width: 100 },
            ...commonHeaders
        ];
        }

        return [];
    }, [entityType]);

    // --- 3. UI RENDERING ---
    return (
    <div className="flex h-full w-full flex-col bg-background">
      
        {/* Basic Toolbar */}
        <div className="flex items-center border-b border-border bg-neutral-950 px-4 py-2 gap-4">
            <span className="text-sm font-semibold text-muted-foreground">Entity:</span>
            <select 
                value={entityType}
                onChange={(e) => setEntityType(e.target.value)}
                className="bg-neutral-900 border border-border text-sm text-foreground rounded px-3 py-1 outline-none focus:border-primary cursor-pointer"
            >
                {availableEntities.length > 0 ? (
                    availableEntities.map((entity) => (
                        <option key={entity} value={entity}>
                            {entity}
                        </option>
                    ))
                ) : (
                    <option value={entityType}>Loading...</option>
                )}
            </select>
            
            <div className="ml-auto text-xs text-muted-foreground font-mono">
                {isLoading ? (
                    <span className="text-primary animate-pulse">Fetching...</span>
                ) : (
                    `${rowData.length} Records`
                )}
            </div>
        </div>

      {/* 2. Use your custom grid component! */}
      <div className="flex-1 w-full">
        <DataGrid
          rowData={rowData}
          columnDefs={colDefs}
        />
      </div>

    </div>
  );

}