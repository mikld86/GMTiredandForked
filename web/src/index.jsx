import './style.css';

import { render } from 'preact';
import { LocationProvider, Router, Route } from 'preact-iso';

import { Header } from './components/Header.jsx';
import { Footer } from './components/Footer.jsx';
import { Home } from './pages/Home/index.jsx';
import { NotFound } from './pages/_404.jsx';
import { Settings } from './pages/Settings/index.jsx';
import { OTA } from './pages/OTA/index.jsx';
import {Scales} from './pages/Scales/index.jsx';

export function App() {
  return (
    <LocationProvider>
      <div className="flex flex-col items-center mt-12 mb-16">
        <Header />
        <Router>
          <Route path="/" component={Home} />
          <Route path="/settings" component={Settings} />
          <Route path="/ota" component={OTA} />
          <Route path="/scales" component={Scales} />
          <Route default component={NotFound} />
        </Router>
        <Footer />
      </div>
    </LocationProvider>
  );
}

render(<App />, document.getElementById('app'));
