import { useState, useEffect } from 'react';
import { DockviewApi, DockviewReact, type DockviewReadyEvent } from 'dockview-react';
import { SidebarProvider } from '@/components/ui/sidebar';
import { TooltipProvider } from '@/components/ui/tooltip';
import { AppSidebar } from './Sidebar';
import { CommandPallete } from './CommandPallete';
import { DOCKVIEW_COMPONENTS, type PanelConfig } from './panels';

export function Workspace() {
    /*
    ----- Client state -----
    */

    const [dockApi, setDockApi] = useState<DockviewApi | null>(null);
    const [isCmdPalleteOpen, setIsCmdPalleteOpen] = useState(false);

    /*
    ----- Global keyboard listeners -----
    */

    useEffect(() => {
        const handleKeyDown = (e: KeyboardEvent) => {
            if (e.key === "k" || e.key === "p" && (e.ctrlKey || e.metaKey)) {
                e.preventDefault();
                setIsCmdPalleteOpen((prev) => !prev);
            }
        };

        document.addEventListener("keydown", handleKeyDown);
        return () => {
            document.removeEventListener("keydown", handleKeyDown);
        };
    }, []);

    // The Master function to open panels - passed down to Sidebar and Cmd Pallete
    const openPanel = (panelConfig: PanelConfig, panelParams?: any) => {
        if (!dockApi)
            return;

        const id = panelConfig.id;
        const title = panelConfig.title;
        const component = panelConfig.componentId;
        
        const panelId = panelConfig.allowMultiple ? `${id}-${crypto.randomUUID().slice(0, 6)}` : id;

        const existingPanel = dockApi.getPanel(panelId);
        if (existingPanel) {
            existingPanel.api.setActive();
        }
        else {
            dockApi.addPanel({
                id: panelId,
                component: component,
                params: { title, ...panelParams },
                title,
            });
        }

        // Close cmd pallete if it was open
        setIsCmdPalleteOpen(false);
    };

    return (
        <div className="flex h-screen w-screen overflow-hidden bg-neutral-950 text-neutral-200">
            <TooltipProvider>
            <SidebarProvider>

                <AppSidebar
                    onOpenPanel={openPanel}
                    onOpenCmdPallete={() => setIsCmdPalleteOpen(true)}
                />

                <CommandPallete
                    open={isCmdPalleteOpen}
                    onOpenChange={setIsCmdPalleteOpen}
                    onNavigate={openPanel}
                />

                <div className="flex-1 relative">
                    <DockviewReact
                        components={DOCKVIEW_COMPONENTS}
                        onReady={(e: DockviewReadyEvent) => setDockApi(e.api)}
                        className="dockview-theme-dark h-full w-full"
                    />
                </div>

            </SidebarProvider>
            </TooltipProvider>
        </div>
    );
}

