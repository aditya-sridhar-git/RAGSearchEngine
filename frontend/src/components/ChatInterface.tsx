import React, { useState, useRef, useEffect } from 'react';
import { MessageBubble } from './MessageBubble';
import './ChatInterface.css';

interface Message {
    id: string;
    text: string;
    sender: 'user' | 'ai';
    timestamp: Date;
}

const API_BASE = 'http://localhost:8080/api';

export const ChatInterface: React.FC = () => {
    const [messages, setMessages] = useState<Message[]>([]);
    const [input, setInput] = useState('');
    const [isLoading, setIsLoading] = useState(false);
    const [searchHistory, setSearchHistory] = useState<string[]>([]);
    const [sidebarOpen, setSidebarOpen] = useState(true);
    const [uploadedDoc, setUploadedDoc] = useState<{ name: string, content: string } | null>(null);
    const [showAnalyzeModal, setShowAnalyzeModal] = useState(false);
    const [analyzeQuery, setAnalyzeQuery] = useState('');
    const [analyzeAction, setAnalyzeAction] = useState<'freq' | 'search' | 'prefix'>('freq');
    const messagesEndRef = useRef<HTMLDivElement>(null);
    const inputRef = useRef<HTMLInputElement>(null);
    const fileInputRef = useRef<HTMLInputElement>(null);

    useEffect(() => {
        const saved = localStorage.getItem('searchHistory');
        if (saved) setSearchHistory(JSON.parse(saved));
    }, []);

    useEffect(() => {
        messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' });
    }, [messages]);

    const saveToHistory = (query: string) => {
        const updated = [query, ...searchHistory.filter(q => q !== query)].slice(0, 15);
        setSearchHistory(updated);
        localStorage.setItem('searchHistory', JSON.stringify(updated));
    };

    const handleSend = async () => {
        if (!input.trim() || isLoading) return;

        const userMessage: Message = {
            id: Date.now().toString(),
            text: input,
            sender: 'user',
            timestamp: new Date(),
        };

        setMessages(prev => [...prev, userMessage]);
        saveToHistory(input);
        const query = input;
        setInput('');
        setIsLoading(true);

        try {
            const response = await fetch(`${API_BASE}/rag`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ query }),
            });

            const data = await response.json();

            const aiMessage: Message = {
                id: (Date.now() + 1).toString(),
                text: data.answer || data.error || 'No response',
                sender: 'ai',
                timestamp: new Date(),
            };

            setMessages(prev => [...prev, aiMessage]);
        } catch (error) {
            const errorMessage: Message = {
                id: (Date.now() + 1).toString(),
                text: 'Failed to connect to server',
                sender: 'ai',
                timestamp: new Date(),
            };
            setMessages(prev => [...prev, errorMessage]);
        } finally {
            setIsLoading(false);
            inputRef.current?.focus();
        }
    };

    const handleFileUpload = async (e: React.ChangeEvent<HTMLInputElement>) => {
        const file = e.target.files?.[0];
        if (!file || !file.name.endsWith('.txt')) {
            alert('Please upload a .txt file');
            return;
        }

        const content = await file.text();
        setUploadedDoc({ name: file.name, content });
        setShowAnalyzeModal(true);
        if (fileInputRef.current) fileInputRef.current.value = '';
    };

    const handleAnalyze = async () => {
        if (!uploadedDoc || !analyzeQuery.trim()) return;

        setShowAnalyzeModal(false);

        const userMessage: Message = {
            id: Date.now().toString(),
            text: `📄 ${uploadedDoc.name}\n\n🔍 ${analyzeAction === 'freq' ? 'Word Frequency' : analyzeAction === 'search' ? 'Keyword Search' : 'Prefix Search'}: "${analyzeQuery}"`,
            sender: 'user',
            timestamp: new Date(),
        };
        setMessages(prev => [...prev, userMessage]);
        setIsLoading(true);

        try {
            const response = await fetch(`${API_BASE}/analyze`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    content: uploadedDoc.content,
                    action: analyzeAction,
                    query: analyzeQuery
                }),
            });

            const data = await response.json();

            let resultText = '';
            if (data.error) {
                resultText = `Error: ${data.error}`;
            } else if (analyzeAction === 'freq') {
                if (data.found) {
                    resultText = `📊 Word Frequency for "${data.word}"\n\n`;
                    resultText += `Total occurrences: ${data.total_freq}\n\n`;
                    resultText += `Document breakdown:\n`;
                    data.documents?.forEach((doc: any) => {
                        resultText += `• ${doc.filename}: ${doc.frequency} times\n`;
                    });
                } else {
                    resultText = `Word "${data.word}" not found in the document.`;
                }
            } else if (analyzeAction === 'search') {
                if (data.found) {
                    resultText = `🔍 Keyword Search: "${data.keyword}"\n\n`;
                    resultText += `Total occurrences: ${data.total_freq}\n\n`;
                    data.results?.forEach((r: any) => {
                        resultText += `• ${r.filename}: ${r.frequency} matches\n`;
                    });
                } else {
                    resultText = `Keyword "${data.keyword}" not found.`;
                }
            } else if (analyzeAction === 'prefix') {
                if (data.found && data.words?.length > 0) {
                    resultText = `🔤 Words with prefix "${data.prefix}":\n\n`;
                    data.words.forEach((w: any) => {
                        resultText += `• ${w.word} (${w.frequency} times)\n`;
                    });
                } else {
                    resultText = `No words found with prefix "${data.prefix}"`;
                }
            }

            const aiMessage: Message = {
                id: (Date.now() + 1).toString(),
                text: resultText || JSON.stringify(data, null, 2),
                sender: 'ai',
                timestamp: new Date(),
            };
            setMessages(prev => [...prev, aiMessage]);
        } catch (error) {
            const errorMessage: Message = {
                id: (Date.now() + 1).toString(),
                text: 'Failed to analyze document',
                sender: 'ai',
                timestamp: new Date(),
            };
            setMessages(prev => [...prev, errorMessage]);
        } finally {
            setIsLoading(false);
            setUploadedDoc(null);
            setAnalyzeQuery('');
        }
    };

    const handleSummarize = async () => {
        if (!uploadedDoc) return;

        setShowAnalyzeModal(false);

        const userMessage: Message = {
            id: Date.now().toString(),
            text: `📄 Uploaded: ${uploadedDoc.name}\n\nSummarize this document.`,
            sender: 'user',
            timestamp: new Date(),
        };
        setMessages(prev => [...prev, userMessage]);
        setIsLoading(true);

        try {
            const response = await fetch(`${API_BASE}/upload`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ content: uploadedDoc.content, filename: uploadedDoc.name, action: 'summarize' }),
            });

            const data = await response.json();

            const aiMessage: Message = {
                id: (Date.now() + 1).toString(),
                text: data.result || data.error || 'No response',
                sender: 'ai',
                timestamp: new Date(),
            };
            setMessages(prev => [...prev, aiMessage]);
        } catch (error) {
            const errorMessage: Message = {
                id: (Date.now() + 1).toString(),
                text: 'Failed to process file',
                sender: 'ai',
                timestamp: new Date(),
            };
            setMessages(prev => [...prev, errorMessage]);
        } finally {
            setIsLoading(false);
            setUploadedDoc(null);
        }
    };

    const handleKeyPress = (e: React.KeyboardEvent) => {
        if (e.key === 'Enter' && !e.shiftKey) {
            e.preventDefault();
            handleSend();
        }
    };

    const clearHistory = () => {
        setSearchHistory([]);
        localStorage.removeItem('searchHistory');
    };

    const newChat = () => {
        setMessages([]);
    };

    return (
        <div className="app-layout">
            {/* Analyze Modal */}
            {showAnalyzeModal && uploadedDoc && (
                <div className="modal-overlay">
                    <div className="modal">
                        <h3>📄 {uploadedDoc.name}</h3>
                        <p className="modal-subtitle">Choose an action:</p>

                        <div className="modal-options">
                            <button onClick={handleSummarize} className="modal-btn summarize">
                                ✨ Summarize (AI)
                            </button>

                            <div className="modal-divider">— or analyze with C engine —</div>

                            <div className="analyze-options">
                                <select
                                    value={analyzeAction}
                                    onChange={(e) => setAnalyzeAction(e.target.value as any)}
                                    className="analyze-select"
                                >
                                    <option value="freq">Word Frequency</option>
                                    <option value="search">Keyword Search</option>
                                    <option value="prefix">Prefix Search</option>
                                </select>
                                <input
                                    type="text"
                                    value={analyzeQuery}
                                    onChange={(e) => setAnalyzeQuery(e.target.value)}
                                    placeholder="Enter word to search..."
                                    className="analyze-input"
                                />
                                <button
                                    onClick={handleAnalyze}
                                    disabled={!analyzeQuery.trim()}
                                    className="modal-btn analyze"
                                >
                                    🔍 Analyze
                                </button>
                            </div>
                        </div>

                        <button onClick={() => setShowAnalyzeModal(false)} className="modal-close">
                            Cancel
                        </button>
                    </div>
                </div>
            )}

            {/* Reopen sidebar button */}
            {!sidebarOpen && (
                <button onClick={() => setSidebarOpen(true)} className="sidebar-reopen-btn">
                    ☰
                </button>
            )}

            {/* Sidebar */}
            <aside className={`sidebar ${sidebarOpen ? 'open' : 'closed'}`}>
                <div className="sidebar-header">
                    <h2>Mini Google</h2>
                    <button onClick={() => setSidebarOpen(!sidebarOpen)} className="toggle-btn">
                        {sidebarOpen ? '◀' : '▶'}
                    </button>
                </div>

                <button onClick={newChat} className="new-chat-btn">
                    + New Chat
                </button>

                <div className="history-section">
                    <div className="history-header">
                        <span>Recent</span>
                        {searchHistory.length > 0 && (
                            <button onClick={clearHistory} className="clear-btn">Clear</button>
                        )}
                    </div>
                    <div className="history-list">
                        {searchHistory.map((query, idx) => (
                            <button
                                key={idx}
                                className="history-item"
                                onClick={() => setInput(query)}
                            >
                                {query.length > 25 ? query.substring(0, 25) + '...' : query}
                            </button>
                        ))}
                    </div>
                </div>
            </aside>

            {/* Main Content */}
            <main className="main-content">
                <div className="messages-area">
                    {messages.length === 0 && (
                        <div className="welcome">
                            <div className="welcome-icon">✦</div>
                            <h1>How can I help you today?</h1>
                            <p className="welcome-hint">Upload a .txt file to analyze with C data structures</p>
                        </div>
                    )}
                    {messages.map(msg => (
                        <MessageBubble key={msg.id} message={msg} />
                    ))}
                    {isLoading && (
                        <div className="loading">
                            <div className="dots"><span></span><span></span><span></span></div>
                        </div>
                    )}
                    <div ref={messagesEndRef} />
                </div>

                <div className="input-area">
                    <div className="input-wrapper">
                        <button
                            onClick={() => fileInputRef.current?.click()}
                            className="attach-btn"
                            title="Upload .txt file for analysis"
                        >
                            +
                        </button>
                        <input
                            type="file"
                            ref={fileInputRef}
                            onChange={handleFileUpload}
                            accept=".txt"
                            hidden
                        />
                        <input
                            ref={inputRef}
                            type="text"
                            value={input}
                            onChange={(e) => setInput(e.target.value)}
                            onKeyPress={handleKeyPress}
                            placeholder="Ask AI or upload a document..."
                            className="chat-input"
                            disabled={isLoading}
                        />
                        <span className="model-badge">phi ▾</span>
                        <button
                            onClick={handleSend}
                            disabled={isLoading || !input.trim()}
                            className="send-btn"
                        >
                            ↑
                        </button>
                    </div>
                </div>
            </main>
        </div>
    );
};
