import React from 'react';
import Display from './Display';
import Read from './Read';
import Navigate from './Navigate.jsx';
import 'bootstrap/dist/css/bootstrap.min.css';
import './css/App.css';
import { BrowserRouter as Router, Route, Routes } from 'react-router-dom';

const App = () => {
  return (
    <Router>
      <div className="App">
        <header className="App-header">
          <h1>Environmental Data Dashboard</h1>
        </header>
        <Navigate />
        <main>
          <Routes>
            <Route path="/read" element={<Read />} />
            <Route path="/display" element={<Display />} />
          </Routes>
        </main>
      </div>
    </Router>
  );
};

export default App;
