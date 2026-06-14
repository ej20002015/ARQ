import { useState } from "react";
import { useQuery } from "@tanstack/react-query";
import { type ColDef } from "ag-grid-community";
import { RefDataAPI } from "@/api/refdata";
import { parseApiError } from "@/api/core/httpClient";
import type { SemanticFormat, RefDataFieldMetadata } from "../types/RefDataFieldMetadata";
import { getGridFormatters } from "@/components/ui/dataGrid/gridFormatters";

const getColType = (arqFormat: SemanticFormat) => {
    switch (arqFormat.toLowerCase()) {
        case "String":      return "text";
        case "Integer":     return "number";
        case "Decimal":     return "number";
        case "Decimal2DP":  return "number";
        case "Decimal4DP":  return "number";
        case "Percentage":  return "number";
        case "BasisPoints": return "number";
        case "Boolean":     return "boolean";
        case "Date":        return "date";
        case "DateTime":    return "date";
        case "UUID":        return "text";
    }
};

export function useRefDataGrid() {
    /*
    ----- Client state -----
    */

    const [entityType, setEntityType] = useState<string | null>(null);

    /*
    ----- Server state -----
    */

    // List of available refdata entities
    const { 
        data: availableEntities = [],
        isError: isEntitiesError,
        error: entitiesError,
        isLoading: isEntitiesLoading,
        refetch: refetchEntities,
    } = useQuery({
        queryKey: ["refdata", "entities"],
        queryFn: RefDataAPI.getEntities,
        staleTime: Infinity, // entities list is unlikely to change often, so we can cache indefinitely
    });

    // Refdata records and associated schema for the currently selected entity
    const {
        data: gridState,
        isLoading: isGridLoading,
        isError: isGridError,
        error: gridError,
        isFetching: isGridFetching,
        dataUpdatedAt,
        refetch: refetchGrid,
    } = useQuery({
        queryKey: ["refdata", entityType, "recordsAndSchema"],

        queryFn: async () => {
            if (!entityType)
                return null;

            const [records, schema] = await Promise.all([
                RefDataAPI.getRecords(entityType!),
                RefDataAPI.getSchema(entityType!),
            ]);

            const mapColumn = (prefix: 'header' | 'data') => (info: RefDataFieldMetadata): ColDef => ({
                field:         `${prefix}.${info.name}`,
                headerName:    info.name,
                headerTooltip: info.comment,
                editable:      !info.uiReadOnly,
                cellDataType:  getColType(info.format),
                ...getGridFormatters(info.format),
            });

            return {
                columnDefs: [
                    ...schema.header.map(mapColumn('header')),
                    ...schema.data.map(mapColumn('data'))
                ],
                rowData: records
            };
        },

        enabled: entityType !== null, // only fetch when an entity is selected
    });

    /*
    ----- Return values -----
    */

    const isError   = isEntitiesError   || isGridError;
    const rawError  = entitiesError     || gridError;
    const isLoading = isEntitiesLoading || isGridLoading;

    return {
        entityType,
        setEntityType,
        availableEntities,
        rowData: gridState?.rowData || [],
        columnDefs: gridState?.columnDefs || [],
        isLoading,
        isError,
        error: rawError ? parseApiError(rawError) : null,
        isFetching: isGridFetching,
        dataUpdatedAt,
        refetch: () => { refetchEntities(); refetchGrid(); },
    }
};