import {
    CommandDialog,
    Command,
    CommandEmpty,
    CommandGroup,
    CommandInput,
    CommandItem,
    CommandList,
} from "@/components/ui/command";
import { PANELS, type PanelConfig } from "./panels";

interface CommandPaletteProps {
    open: boolean;
    onOpenChange: (open: boolean) => void;
    onNavigate: (panelConfig: PanelConfig, params?: any) => void;
}

const commandItemClassName = "data-[selected=true]:bg-primary data-[selected=true]:text-primary-foreground cursor-pointer";

export function CommandPallete({ open, onOpenChange, onNavigate }: CommandPaletteProps) {
    const executeNavigation = (panelConfig: PanelConfig) => {
        onNavigate(panelConfig);
        onOpenChange(false);
    };

    return (
        <CommandDialog open={open} onOpenChange={onOpenChange}>
            <Command>
                <CommandInput placeholder="Type a command or search panels..." />
                <CommandList>
                    <CommandEmpty>No results found.</CommandEmpty>
                    
                    {/* Map directly over the configuration to create flat groups */}
                    {PANELS.map((group) => {
                        const Icon = group.icon;
                        
                        return (
                            <CommandGroup key={group.id} heading={group.title}>
                                {group.panels.map((panel) => (
                                    <CommandItem 
                                        key={panel.id} 
                                        className={commandItemClassName}
                                        onSelect={() => executeNavigation(panel)}
                                    >
                                        <Icon className="mr-2 h-4 w-4 opacity-70" />
                                        <span>{panel.title}</span>
                                    </CommandItem>
                                ))}
                            </CommandGroup>
                        );
                    })}
                    
                </CommandList>
            </Command>
        </CommandDialog>
    );
};