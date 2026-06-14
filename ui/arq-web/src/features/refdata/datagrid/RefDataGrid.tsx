import { DataGrid } from "@/components/ui/dataGrid/dataGrid";
import { Button } from "@/components/ui/button";
import {
  Combobox,
  ComboboxContent,
  ComboboxEmpty,
  ComboboxInput,
  ComboboxItem,
  ComboboxList,
} from "@/components/ui/combobox"
import { RefreshCw } from "lucide-react";
import { ErrorState } from "@/components/ui/error/errorState";
import { useRefDataGrid } from "./useRefDataGrid";

export function RefDataGrid() {
    const { 
        entityType,
        setEntityType,
        availableEntities,
        rowData,
        columnDefs,
        isLoading,
        isError,
        error,
        isFetching,
        dataUpdatedAt,
        refetch,
    } = useRefDataGrid();

    const lastSyncTime = dataUpdatedAt 
        ? new Date(dataUpdatedAt).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' }) 
        : "Never";

    return (
        <div className="flex h-full w-full flex-col bg-background">

            {/* Basic Toolbar */}
            <div className="flex items-center border-b border-border bg-neutral-950 px-4 py-2 gap-4">
                <span className="text-sm font-semibold text-muted-foreground">Entity:</span>
                
                <Combobox 
                    items={availableEntities || []}
                    value={entityType}
                    onValueChange={setEntityType}
                    disabled={!availableEntities || availableEntities.length === 0}
                    autoHighlight
                >
                    <ComboboxInput 
                        placeholder="Select entity..."
                        className="w-[200px] h-8 bg-neutral-900 border-border text-xs font-normal"
                    />
                    
                    <ComboboxContent className="w-[200px] p-0 border-border bg-neutral-950">
                        <ComboboxEmpty className="py-2 text-center text-xs text-muted-foreground">
                            No entity found
                        </ComboboxEmpty>
                        
                        <ComboboxList className="space-y-0.5">
                            {(entity: string) => (
                                <ComboboxItem 
                                    key={entity}
                                    value={entity}
                                    className="text-xs cursor-pointer aria-selected:bg-primary data-[highlighted]:bg-primary data-[highlighted]:text-primary-foreground"
                                >
                                    {entity}
                                </ComboboxItem>
                            )}
                        </ComboboxList>
                    </ComboboxContent>
                </Combobox>

                <div className="ml-auto flex items-center gap-4 text-xs font-mono">
                    {/* Status Text */}
                    {isLoading ? (
                        <span className="text-primary animate-pulse">Initializing...</span>
                    ) : isError ? (
                        <span className="text-destructive">Sync Failed!</span>
                    ) : (
                        <div className="flex items-center gap-3 text-muted-foreground">
                            <span>{rowData.length} Records</span>
                            <span className="text-border">|</span>
                            <span>Last pulled: {lastSyncTime}</span>
                        </div>
                    )}

                    {/* Resync Button */}
                    <Button
                        variant="outline"
                        size="icon"
                        className="h-8 w-8 bg-neutral-900 border-border hover:bg-neutral-800"
                        onClick={() => refetch()}
                        disabled={isLoading || isFetching || !entityType}
                        title="Resync Data"
                    >
                        {/* The icon will spin smoothly while a background fetch is happening */}
                        <RefreshCw className={`h-4 w-4 text-muted-foreground ${isFetching ? 'animate-spin text-primary' : ''}`} />
                    </Button>
                </div>
            </div>

        {/* Grid */}
        <div className="flex-1 w-full">
            {isError ? (
                <ErrorState 
                    title="Failed to load reference data"
                    message={error || "Unknown network failure"}
                    onRetry={() => refetch()}
                    isRetrying={isFetching}
                />
            ) : (
                <DataGrid rowData={rowData} columnDefs={columnDefs} />
            )}
        </div>

    </div>
  );

}