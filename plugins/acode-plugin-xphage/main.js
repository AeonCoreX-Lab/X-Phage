class XPhagePlugin {
    async init() {
        const editorManager = acode.require('editorManager');
        
        // ১. Ace Highlight Rules ডিফাইন করা
        ace.define("ace/mode/xphage_highlight_rules", ["require", "exports", "module", "ace/lib/oop", "ace/mode/text_highlight_rules"], function(require, exports, module) {
            "use strict";
            var oop = require("ace/lib/oop");
            var TextHighlightRules = require("ace/mode/text_highlight_rules").TextHighlightRules;
            var XPhageHighlightRules = function() {
                this.$rules = {
                    "start": [
                        { token: "keyword.control", regex: "\\b(pulse|core|scan|bypass|~link|quantum|vortex|synapse|chronos|ether|void|global|fusion)\\b" },
                        { token: "entity.name.type.ui", regex: "@[a-zA-Z_]+" },
                        { token: "support.class", regex: "\\b(Signal|Vision|Orbit|Trigger|Vortex)\\b" },
                        { token: "storage.type", regex: "\\b(atom|shadow|matrix)\\b" },
                        { token: "support.function", regex: "\\b(beam)\\b" },
                        { token: "string", regex: '"', next: "string" },
                        { token: "comment", regex: "//.*$" },
                        { token: "constant.numeric", regex: "\\b[0-9]+(\\.[0-9]+)?\\b" },
                        { token: "keyword.operator", regex: "\\+|\\-|\\*|\\/|=|->|~" }
                    ],
                    "string": [
                        { token: "constant.character.escape", regex: "\\\\." },
                        { token: "string", regex: '"', next: "start" },
                        { defaultToken: "string" }
                    ]
                };
            };
            oop.inherits(XPhageHighlightRules, TextHighlightRules);
            exports.XPhageHighlightRules = XPhageHighlightRules;
        });

        // ২. Ace Mode ডিফাইন করা
        ace.define("ace/mode/xphage", ["require", "exports", "module", "ace/lib/oop", "ace/mode/text", "ace/mode/xphage_highlight_rules"], function(require, exports, module) {
            "use strict";
            var oop = require("ace/lib/oop");
            var TextMode = require("ace/mode/text").Mode;
            var XPhageHighlightRules = require("ace/mode/xphage_highlight_rules").XPhageHighlightRules;
            var Mode = function() { this.HighlightRules = XPhageHighlightRules; };
            oop.inherits(Mode, TextMode);
            (function() { this.$id = "ace/mode/xphage"; }).call(Mode.prototype);
            exports.Mode = Mode;
        });

        // ৩. জোরপূর্বক মোড সেট করার ফাংশন
        const forceApplyMode = () => {
            const activeFile = editorManager.activeFile;
            if (activeFile && activeFile.name) {
                const name = activeFile.name.toLowerCase();
                if (name.endsWith('.xp0') || name.endsWith('.xh') || name.endsWith('.xui')) {
                    activeFile.session.setMode('ace/mode/xphage');
                }
            }
        };

        // ৪. সব ধরনের ইভেন্টে লিসেনার লাগানো
        editorManager.on('switch-file', forceApplyMode);
        editorManager.on('rename-file', forceApplyMode);
        
        // ৫. একটি ছোট ইন্টারভাল রাখা যাতে কোনোভাবেই মিস না হয়
        this.checkInterval = setInterval(forceApplyMode, 1000);
        
        forceApplyMode();
    }

    async destroy() {
        const editorManager = acode.require('editorManager');
        if (this.checkInterval) clearInterval(this.checkInterval);
        editorManager.off('switch-file');
    }
}

if (window.acode) {
    const xphage = new XPhagePlugin();
    acode.setPluginInit("com.aeoncorex.xphage", async () => {
        await xphage.init();
    });
    acode.setPluginUnmount("com.aeoncorex.xphage", () => {
        xphage.destroy();
    });
}
