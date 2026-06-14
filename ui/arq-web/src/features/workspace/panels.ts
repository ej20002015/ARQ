
import { type LucideIcon, Table, ChartCandlestick, Database} from "lucide-react";
import { PanelPlaceholder } from "./PanelPlaceholder"
import { RefDataGrid } from "../refdata/datagrid/RefDataGrid"

export interface PanelConfig {
    id: string;
    title: string;
    componentId: string;
    component: React.FC<any>;
    allowMultiple: boolean;
}

interface PanelGroup {
    id: string;
    title: string;
    icon: LucideIcon;
    panels: PanelConfig[];
}

export const PANELS : PanelGroup[] = [
    {
        id: "trading",
        title: "Trading",
        icon: Table,
        panels: [
            {
                id: "blotter",
                title: "Trade Blotter",
                componentId: "placeholder",
                component: PanelPlaceholder,
                allowMultiple: true,
            },
        ],
    },
    {
        id: "market-data",
        title: "Market Data",
        icon: ChartCandlestick,
        panels: [
            {
                id: "fxRates",
                title: "FX Rates",
                componentId: "placeholder", 
                component: PanelPlaceholder,
                allowMultiple: true,
            },
            {
                id: "curveBuilder",
                title: "Curve Builder",
                componentId: "placeholder", 
                component: PanelPlaceholder,
                allowMultiple: false,
            }
        ]
    },
    {
        id: "reference-data",
        title: "Reference Data",
        icon: Database,
        panels: [
            {
                id: "refdata",
                title: "Reference Data Grid",
                componentId: "refDataGrid",
                component: RefDataGrid,
                allowMultiple: true,
            }
        ]
    },
];

export const DOCKVIEW_COMPONENTS = PANELS
    .flatMap(group => group.panels)
    .reduce((acc, panel) => {
        acc[panel.componentId] = panel.component;
        return acc;
    }, {} as Record<string, React.FC<any>>);