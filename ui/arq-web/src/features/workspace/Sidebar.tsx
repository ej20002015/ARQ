import {
    Search,
    ChevronRight,
    ArrowRightFromLine
} from "lucide-react";
import {
    Collapsible,
    CollapsibleContent,
    CollapsibleTrigger,
} from "@/components/ui/collapsible"
import {
    Sidebar,
    SidebarContent,
    SidebarFooter,
    SidebarGroup,
    SidebarHeader,
    SidebarMenu,
    SidebarMenuButton,
    SidebarMenuItem,
    SidebarMenuSub,
    SidebarMenuSubButton,
    SidebarMenuSubItem,
    useSidebar,
} from "@/components/ui/sidebar";
import { PANELS, type PanelConfig } from "./panels";
import arqLogo from "../../assets/arqLogoSideup.svg";

interface AppSidebarProps {
    onOpenPanel: (panelConfig: PanelConfig) => void;
    onOpenCmdPallete: () => void;
}

export function AppSidebar({ onOpenPanel, onOpenCmdPallete }: AppSidebarProps) {
    const { state, toggleSidebar } = useSidebar();

    return (
        <Sidebar collapsible="icon" className="border-r border-border bg-neutral-950">
        
        {/* --- HEADER (Logo & Toggle) --- */}
        <SidebarHeader className="h-17 flex justify-center border-b border-border">
            <SidebarMenu>
                <SidebarMenuItem>
                    <SidebarMenuButton size="lg" onClick={toggleSidebar} className="hover:bg-transparent">
                    {state === "expanded" ? (
                        <img
                            src={arqLogo}
                            alt="ARQ"
                            className="h-10 w-auto m-auto rounded-sm"
                        />
                    ) : (
                        <div className="flex h-8 w-8 items-center justify-center rounded-md bg-primary/20 text-primary">
                        <ArrowRightFromLine size={18} />
                        </div>
                    )}
                    </SidebarMenuButton>
                </SidebarMenuItem>
            </SidebarMenu>
        </SidebarHeader>

        {/* --- MAIN NAVIGATION CONTENT --- */}
        <SidebarContent>
            <SidebarGroup>
                <SidebarMenu>
                    {PANELS.map((group) => {
                        const Icon = group.icon;
                        return (
                            <Collapsible
                                key={group.id}
                                defaultOpen={false}
                                className="group/collapsible"
                            >
                                <SidebarMenuItem>
                                    <CollapsibleTrigger render={
                                        <SidebarMenuButton className="text-foreground hover:text-foreground">
                                            <Icon />
                                            <span>{group.title}</span>
                                            <ChevronRight className="ml-auto transition-transform in-data-closed:rotate-0 in-data-open:rotate-90 duration-200" />
                                        </SidebarMenuButton>
                                    }/>

                                    {/* The Indented Child Panels */}
                                    <CollapsibleContent>
                                        <SidebarMenuSub>
                                            {group.panels.map((panel) => (
                                                <SidebarMenuSubItem key={panel.id}>
                                                    <SidebarMenuSubButton
                                                        onClick={() => onOpenPanel(panel)}
                                                        className="text-muted-foreground hover:text-primary cursor-pointer"
                                                        render={
                                                            <span>{panel.title}</span>
                                                        }/>
                                                </SidebarMenuSubItem>
                                            ))}
                                        </SidebarMenuSub>
                                    </CollapsibleContent>
                                </SidebarMenuItem>
                            </Collapsible>
                        );
                    })}
                </SidebarMenu>
            </SidebarGroup>
        </SidebarContent>

        {/* --- FOOTER (Command Palette) --- */}
        <SidebarFooter className="border-t border-border">
            <SidebarMenu>
                <SidebarMenuItem>
                    <SidebarMenuButton 
                        onClick={onOpenCmdPallete} 
                        tooltip="Search Commands"
                        className="bg-primary text-primary-foreground hover:bg-primary/90 hover:text-primary-foreground transition-all shadow-sm"
                    >
                        <Search className="h-4 w-4 shrink-0" />
                        <span className="font-medium">Search Commands</span>
                        <kbd className="ml-auto hidden sm:inline-block bg-background rounded px-1.5 font-mono">
                            Ctrl+K
                        </kbd>
                    </SidebarMenuButton>
                </SidebarMenuItem>
            </SidebarMenu>
        </SidebarFooter>
        </Sidebar>
    );
}