import type { ColDef } from 'ag-grid-community';
import type { SemanticFormat } from '@/features/refdata/types/RefDataFieldMetadata';
import * as Formatters from '@/core/formatters';

export function getGridFormatters(format: SemanticFormat): Partial<ColDef> {
    switch (format) {
        case 'String':
        case 'UUID':
            return { cellDataType: 'text' };
            
        case 'Boolean':
            return { cellDataType: 'boolean' };
            
        case 'Date':
            return { 
                cellDataType: 'date',
                valueFormatter: (params) => Formatters.formatDate(params.value)
            };
            
        case 'DateTime':
            return { 
                cellDataType: 'date',
                valueFormatter: (params) => Formatters.formatDateTime(params.value)
            };
        
        case 'Integer':
            return {
                type: 'numericColumn',
                cellDataType: 'number',
                valueFormatter: (params) => Formatters.formatDecimal(params.value, 0)
            };
            
        case 'Decimal':
            return { type: 'numericColumn', cellDataType: 'number' };
            
        case 'Decimal2DP':
            return {
                type: 'numericColumn',
                cellDataType: 'number',
                valueFormatter: (params) => Formatters.formatDecimal(params.value, 2)
            };
            
        case 'Decimal4DP':
            return {
                type: 'numericColumn',
                cellDataType: 'number',
                valueFormatter: (params) => Formatters.formatDecimal(params.value, 4)
            };
            
        case 'Percentage':
            return {
                type: 'numericColumn',
                cellDataType: 'number',
                valueFormatter: (params) => Formatters.formatPercentage(params.value)
            };
            
        case 'BasisPoints':
            return {
                type: 'numericColumn',
                cellDataType: 'number',
                valueFormatter: (params) => Formatters.formatBps(params.value)
            };
            
        default:
            return { cellDataType: 'text' };
    }
}