import React from "react";
import RepositoryList from "./components/RepositoryList/RepositoryList";
import './App.css';

function App() {
  return (
    <div className="App">
            <header className="App-header">
                <h1>MyGit Web UI</h1>
            </header>
            <main>
                <RepositoryList />
            </main>
        </div>
  );
}

export default App;
