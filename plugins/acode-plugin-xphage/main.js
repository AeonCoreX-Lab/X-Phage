import modeXPhage from './mode-xphage.js';

class XPhagePlugin {
    async init() {
        const { editorManager } = acode;

        // Register the language mode
        editorManager.registerMode('xphage', modeXPhage);

        // Link .xp0 extension to this mode
        editorManager.on('add-file', (file) => {
            if (file.name.endsWith('.xp0')) {
                file.setMode('ace/mode/xphage');
            }
        });
    }

    async destroy() {
        // Clean up when plugin is disabled
    }
}

if (window.acode) {
    const xphagePlugin = new XPhagePlugin();
    acode.setPluginInit(xphagePlugin.id, async (baseUrl, $page, { cacheFile, cacheFileUrl }) => {
        await xphagePlugin.init();
    });
    acode.setPluginUnmount(xphagePlugin.id, async () => {
        await xphagePlugin.destroy();
    });
}
