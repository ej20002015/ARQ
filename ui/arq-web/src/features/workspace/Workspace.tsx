import { useState } from 'react';
import { DockviewApi, DockviewReact, type DockviewReadyEvent } from 'dockview-react';
import { Sidebar } from './Sidebar';
// Panels
import { RefDataGrid } from '../refdata/datagrid/RefDataGrid';

function PanelPlaceholder(props: any) {
    const title = props.params.title || "Placeholder Panel";
    return (
        <div className="flex h-full w-full flex-col items-center justify-center bg-neutral-950 text-neutral-200 p-4 select-none">
            <div className="text-center">
                <p className="text-lg font-semibold">{title}</p>
                <p className="text-xs text-neutral-500 font-mono mt-1">ID: {props.api.id}</p>
            </div>
        </div>
    );
}

const components = {
    placeholder: PanelPlaceholder,
    refDataGrid: RefDataGrid
};

export function Workspace() {

    // State
    const [dockApi, setDockApi] = useState<DockviewApi | null>(null);
    const [isSidebarOpen, setSidebarOpen] = useState(true);
    const [isCmdkOpen, setCmdkOpen] = useState(false);

    // The Master function to open panels - passed down to Sidebar and Cmdk
    const openPanel = (id: string, title: string, componentName: string = "placeholder") => {
        if (!dockApi)
            return;
        
        const existingPanel = dockApi.getPanel(id);
        if (existingPanel) {
            existingPanel.api.setActive();
        } else {
            dockApi.addPanel({
                id,
                component: componentName,
                params: { title },
                title,
            });
        }

        // Close Cmdk if it was open
        setCmdkOpen(false);
    };

    return (
        <div className="flex h-screen w-screen overflow-hidden bg-neutral-950 text-neutral-200">

            <Sidebar
                isOpen={isSidebarOpen}
                onToggle={() => setSidebarOpen(!isSidebarOpen)}
                onOpenPanel={openPanel}
                onOpenCmdk={() => setCmdkOpen(true)}
            />

            <div className="flex-1 relative">
                <DockviewReact
                    components={components}
                    onReady={(e: DockviewReadyEvent) => setDockApi(e.api)}
                    className="dockview-theme-dark h-full w-full"
                />
            </div>

        </div>
    );
}

